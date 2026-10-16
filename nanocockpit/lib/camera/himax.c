/*
 * himax.c
 * Elia Cereda <elia.cereda@idsia.ch>
 *
 * Copyright (C) 2022-2025 IDSIA, USI-SUPSI
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * This software is based on the following publication:
 *    E. Cereda, A. Giusti, D. Palossi. "NanoCockpit: Performance-optimized 
 *    Application Framework for AI-based Autonomous Nanorobotics"
 * We kindly ask for a citation if you use in academic work.
 */

#include "camera/himax.h"

#include "config.h"
#include "camera.h"
#include "camera/himax_defs.h"
#include "debug.h"

#include <pmsis.h>
#include <bsp/camera/himax.h>

#include <stdint.h>

static inline uint8_t himax_reg_get8(pi_device_t *camera, uint16_t reg_addr) {
    uint8_t value;
    pi_camera_reg_get(camera, reg_addr, &value);

#ifdef HIMAX_REG_DUMP
    DEBUG_PRINT("HIMAX reg 0x%04x = 0x%02x\n", reg_addr, value);
#endif

    return value;
}

static inline void himax_reg_set8(pi_device_t *camera, uint16_t reg_addr, uint8_t new_value) {
    pi_camera_reg_set(camera, reg_addr, &new_value);

#ifdef HIMAX_REG_VALIDATE
    uint8_t current_value = himax_reg_get8(camera, reg_addr);

    if(new_value != current_value) {
        CO_ASSERTION_FAILURE("HIMAX register 0x%04x not set correctly (set 0x%02x, got 0x%02x)!\n", reg_addr, new_value, current_value);
    }
#endif
}

static inline void himax_reg_set16(pi_device_t *camera, uint16_t reg_addr_h, uint16_t new_value) {
    uint16_t reg_addr_l = reg_addr_h + 1;
    uint8_t value_h = (0xFF00 & new_value) >> 8;
    uint8_t value_l = (0x00FF & new_value);

    himax_reg_set8(camera, reg_addr_h, value_h);
    himax_reg_set8(camera, reg_addr_l, value_l);
}

static inline uint16_t himax_reg_get16(pi_device_t *camera, uint16_t reg_addr_h) {
    uint16_t reg_addr_l = reg_addr_h + 1;
    uint8_t value_h = himax_reg_get8(camera, reg_addr_h);
    uint8_t value_l = himax_reg_get8(camera, reg_addr_l);
    return (value_h << 8) | value_l;
}

void himax_set_mode(himax_t *himax, himax_mode_e mode) {
    himax_reg_set8(&himax->camera, HIMAX_MODE_SELECT, (uint8_t)mode);
    himax->current_mode = mode;
}

uint8_t himax_get_frame_count(himax_t *himax) {
    return himax_reg_get8(&himax->camera, HIMAX_FRAME_COUNT);
}

int32_t himax_init(himax_t *himax) {
    int32_t status;

#if defined(__PLATFORM_GVSOC__)
    // GVSOC does not support PWM and does not really simulate the clock anyway
    VERBOSE_PRINT("HIMAX clock mode:\t\tGVSOC @ %dMHz /%d\n", HIMAX_FQCY/1000000, vt_div[HIMAX_SYS_DIV]);
#elif HIMAX_ANA == 0 // MCLK mode
    struct pi_pwm_conf pwm_conf = {0};

    pi_pwm_conf_init(&pwm_conf);
    pwm_conf.pwm_id = 0x00; // timer0
    pwm_conf.ch_id = 0x00;  // channel0
    pwm_conf.timer_conf &= ~PI_PWM_CLKSEL_REFCLK_32K;
    pwm_conf.timer_conf |= PI_PWM_CLKSEL_FLL;
    pi_open_from_conf(&himax->mclk_timer, &pwm_conf);

    status = pi_pwm_open(&himax->mclk_timer); 

    if (status) {
        return status;
    }

    pi_pwm_duty_cycle_set(&himax->mclk_timer, HIMAX_FQCY, 50);
    pi_pwm_timer_start(&himax->mclk_timer);

    VERBOSE_PRINT("HIMAX clock mode:\t\tMCLK @ %dMHz /%d\n", HIMAX_FQCY/1000000, vt_div[HIMAX_SYS_DIV]);
#else // OSC mode
    VERBOSE_PRINT("HIMAX clock mode:\t\tOSC @ %dMHz /%d\n", HIMAX_FQCY/1000000, vt_div[HIMAX_SYS_DIV]);
#endif

    struct pi_himax_conf camera_conf = {0};

    pi_himax_conf_init(&camera_conf);

    pi_open_from_conf(&himax->camera, &camera_conf);

    status = pi_camera_open(&himax->camera);

    if (status) {
        return status;
    }

    himax->current_mode = HIMAX_MODE_UNKNOWN;

    return status;
}

static uint16_t min_line_len_pck(himax_format_e format) {
    switch (format) {
    case HIMAX_FULL: 
        return 0x0178; // 376
    case HIMAX_QVGA:
        return 0x0178; // 376 
    case HIMAX_HALF:
        // Note: HALF not defined in the datasheet, we assume equal to QQVGA.
        return 0x00D7; // 215
    case HIMAX_QQVGA:
        return 0x00D7; // 215
    default:
        CO_ASSERTION_FAILURE("HIMAX camera format %d not supported!\n", format);
    }
}

static uint16_t min_frame_len_lines(himax_format_e format) {
    switch (format) {
    case HIMAX_FULL:
        return 0x0158; // 344
    case HIMAX_QVGA:
        return 0x0104; // 260
    case HIMAX_HALF:
        // Note: HALF not defined in the datasheet, we assume proportional to the others
        return 0x00AA; // 170
    case HIMAX_QQVGA:
        return 0x0080; // 128
    default:
        CO_ASSERTION_FAILURE("HIMAX camera format %d not supported!\n", format);
    }
}

static uint32_t vt_pix_clk() {
    return HIMAX_FQCY / vt_div[HIMAX_SYS_DIV];
}

static uint16_t compute_line_len_pck(himax_format_e format, float frame_rate) {
    // Compute the best line_len_pck for the desired frame_rate, given a certain format
    // TODO: not sure using floats is the best choice to accurately represent a frame rate
    uint32_t frame_len_pck = vt_pix_clk() / frame_rate;

    uint16_t line_len_pck = min_line_len_pck(format);
    for (; line_len_pck < frame_len_pck; line_len_pck++) {
        if (frame_len_pck % line_len_pck == 0) {
            break;
        }
    }

    return line_len_pck;
}

static uint16_t compute_frame_len_lines(himax_format_e format, uint16_t line_len_pck, float frame_rate) {
    // Compute frame_len_lines from a desired frame_rate, given a certain format and line_len_pck
    uint16_t frame_len_lines = vt_pix_clk() / (frame_rate * line_len_pck);
    frame_len_lines = MAX(min_frame_len_lines(format), frame_len_lines);

    return frame_len_lines;
}

static float compute_frame_rate(uint16_t frame_len_lines, uint16_t line_len_pck) {
    return ((float)vt_pix_clk()) / (frame_len_lines * line_len_pck);
}

static uint16_t compute_integration_lines(uint16_t frame_len_lines, uint16_t line_len_pck, float integration_ms) {
    // Compute integration_lines from a desired integration period in ms, given frame_len_lines and line_len_pck
    uint16_t integration_lines = (integration_ms / 1000) * vt_pix_clk() / line_len_pck;
    integration_lines = MAX(2, MIN(integration_lines, frame_len_lines - 2));
    
    return integration_lines;
}

static float compute_integration_ms(uint16_t integration_lines, uint16_t line_len_pck) {
    return (float)(integration_lines * line_len_pck) / vt_pix_clk() * 1000;
}

static int compute_analog_gain(uint8_t analog_gain) {
  // According to the datasheet, analog gain is represented as log2(x) in bits [6:4]:
  // 0x00: 1x, 0x10: 2x, 0x20: 4x, 0x30: 8x, 0x40: 16x
  analog_gain = (analog_gain & 0x70) >> 4;
  return 1 << analog_gain;
}

static float compute_digital_gain(uint16_t digital_gain) {
  // According to the datasheet, digital gain is stored as a 2-bit int, 6-bit
  // float mixed format, with a maximum gain of 4x. I'm assuming the two parts
  // are simply summed, the int part represents [0, 3] and the float [0, 1.0].
  // TODO: to verify, test if int=1,float=0 (0.0) and int=0,float=63 (1.0) 
  // result in the same images.
  // Maybe the test patterns can be used to test this reliably?
  uint8_t integer_gain = (digital_gain & 0x300) >> 8;
  uint8_t float_gain = (digital_gain & 0xFC) >> 2;
  return integer_gain + float_gain / (float)((1 << 6) - 1);
}

void himax_configure(himax_t *himax) {
    himax_format_e format = HIMAX_FORMAT;

    uint8_t image_orientation = HIMAX_ORIENTATION;

    float desired_frame_rate = HIMAX_FRAME_RATE;
    uint16_t    line_len_pck = compute_line_len_pck(format, desired_frame_rate);
    uint16_t frame_len_lines = compute_frame_len_lines(format, desired_frame_rate, line_len_pck);

    uint16_t integration_lines = compute_integration_lines(frame_len_lines, line_len_pck, HIMAX_INTEGRATION_MS);
    uint8_t        analog_gain = HIMAX_AGAIN;
    uint16_t      digital_gain = HIMAX_DGAIN;

    uint8_t readout_x, readout_y, binning_mode;

    uint8_t ae_ctrl = HIMAX_AE;

    uint8_t qvga_enable;

    uint8_t osc_clk_div = vt_sys_reg_div_lut[HIMAX_REG_DIV][HIMAX_SYS_DIV];
    uint8_t  ana_reg_17 = HIMAX_ANA;

    switch(format) {
    case HIMAX_FULL:
        readout_x = 0x01;
        readout_y = 0x01;
        binning_mode = 0x00;
        qvga_enable = 0x00;
        break;

    case HIMAX_QVGA:
        readout_x = 0x01;
        readout_y = 0x01;
        binning_mode = 0x00;
        qvga_enable = 0x01;
        break;

     case HIMAX_HALF:
        readout_x = 0x03;
        readout_y = 0x03;
        binning_mode = 0x03;
        qvga_enable = 0x00;
        break;

    case HIMAX_QQVGA:
        readout_x = 0x03;
        readout_y = 0x03;
        binning_mode = 0x03;
        qvga_enable = 0x01; // TODO: should be 0x01 in theory, but setting 0x00 and cropping in software is faster
        break;
        
    default:
        CO_ASSERTION_FAILURE("HIMAX camera format %d not supported!\n", format);
    }

    VERBOSE_PRINT(
        "HIMAX format:\t\t\t%d (%d x %dpx)\n",
        HIMAX_FORMAT,
        HIMAX_HEIGHT, HIMAX_WIDTH
    );

    float actual_frame_rate = compute_frame_rate(frame_len_lines, line_len_pck);
    VERBOSE_PRINT(
        "HIMAX frame timings:\t\t%.2ffps (%d x %d @ %luMHz)\n",
        actual_frame_rate,
        frame_len_lines, line_len_pck, vt_pix_clk() / 1000000
    );

    float actual_integration_time = compute_integration_ms(integration_lines, line_len_pck);
    VERBOSE_PRINT(
        "HIMAX exposure:\t\t\tAE %d, INTG %.2fms (%d x %d @ %luMHz), AGAIN %dx (0x%02x), DGAIN %.2fx (0x%04x)\n",
        ae_ctrl, actual_integration_time,
        integration_lines, line_len_pck, vt_pix_clk() / 1000000,
        compute_analog_gain(analog_gain), analog_gain, compute_digital_gain(digital_gain), digital_gain
    );

    pi_device_t *camera = &himax->camera;

    // Ensure the camera is in STANDBY mode and wait until it has finished processing 
    // the last frame before changing the following registers.
    himax_set_mode(himax, HIMAX_MODE_STANDBY);
    pi_time_wait_us(50000);

    /*********************** SENSOR MODE CONTROL **********************/
    himax_reg_set8(camera, HIMAX_IMG_ORIENTATION, image_orientation);

    /****************** SENSOR EXPOSURE GAIN CONTROL ******************/
    himax_reg_set16(camera, HIMAX_INTEGRATION_H, integration_lines);
    himax_reg_set8( camera, HIMAX_ANALOG_GAIN, analog_gain);
    himax_reg_set16(camera, HIMAX_DIGITAL_GAIN_H, digital_gain);

    /********************** FRAME TIMING CONTROL **********************/
    himax_reg_set16(camera, HIMAX_FRAME_LEN_LINES_H, frame_len_lines);
    himax_reg_set16(camera, HIMAX_LINE_LEN_PCK_H, line_len_pck);

    /********************** BINNING MODE CONTROL **********************/
    himax_reg_set8( camera, HIMAX_READOUT_X, readout_x);
    himax_reg_set8( camera, HIMAX_READOUT_Y, readout_y);
    himax_reg_set8( camera, HIMAX_BINNING_MODE, binning_mode);

    /********************** TEST PATTERN CONTROL **********************/
    // himax_reg_set8( camera, HIMAX_TEST_PATTERN_MODE, 0x01); // Color bar
    // himax_reg_set8( camera, HIMAX_TEST_PATTERN_MODE, 0x11); // Walking 1

    /**************** VSYNC, HSYNC AND PIXEL SHIFT REGISTER ***************/
    // Avoid invalid first two pixels, correctly aligning capture with start of frame
    himax_reg_set8( camera, HIMAX_VSYNC_HSYNC_PIXEL_SHIFT_EN, 0x01);

    /***************** AUTOMATIC EXPOSURE GAIN CONTROL ****************/
    himax_reg_set8( camera, HIMAX_AE_CTRL, ae_ctrl);
    // himax_reg_set8( camera, HIMAX_AE_TARGET_MEAN, ae_target_mean);
    // himax_reg_set8( camera, HIMAX_FS_CTRL, ae_fs_ctrl);

    /********************** SENSOR TIMING CONTROL *********************/
    himax_reg_set8( camera, HIMAX_QVGA_WIN_EN, qvga_enable);

    /********************** IO AND CLOCK CONTROL **********************/
    himax_reg_set8( camera, HIMAX_OSC_CLK_DIV, osc_clk_div);
    himax_reg_set8( camera, HIMAX_ANA_Register_17, ana_reg_17);

    // Commit register changes
    himax_reg_set8( camera, HIMAX_GRP_PARAM_HOLD, 0x01);
}

void himax_dump_config(himax_t *himax) {
    uint16_t frame_len_lines = himax_reg_get16(&himax->camera, HIMAX_FRAME_LEN_LINES_H);
    uint16_t line_len_pck = himax_reg_get16(&himax->camera, HIMAX_LINE_LEN_PCK_H);
    float actual_frame_rate = compute_frame_rate(frame_len_lines, line_len_pck);
    VERBOSE_PRINT(
        "HIMAX current frame timings:\t%.2ffps (%d x %d @ %luMHz)\n",
        actual_frame_rate,
        frame_len_lines, line_len_pck, vt_pix_clk() / 1000000
    );

    uint8_t ae_ctrl = himax_reg_get8(&himax->camera, HIMAX_AE_CTRL);
    uint16_t integration_lines = himax_reg_get16(&himax->camera, HIMAX_INTEGRATION_H);
    uint8_t analog_gain = himax_reg_get8(&himax->camera, HIMAX_ANALOG_GAIN);
    uint16_t digital_gain = himax_reg_get16(&himax->camera, HIMAX_DIGITAL_GAIN_H);
    float actual_integration_time = compute_integration_ms(integration_lines, line_len_pck);
    VERBOSE_PRINT(
        "HIMAX current exposure:\t\tAE %d, INTG %.2fms (%d x %d @ %luMHz), AGAIN %dx (0x%02x), DGAIN %.2fx (0x%04x)\n",
        ae_ctrl, actual_integration_time,
        integration_lines, line_len_pck, vt_pix_clk() / 1000000,
        compute_analog_gain(analog_gain), analog_gain, compute_digital_gain(digital_gain), digital_gain
    );
}

typedef struct {
    uint16_t addr;
    uint8_t value;
} i2c_req_t;

typedef struct {
    struct pi_himax_conf conf;
    struct pi_device cpi_device;
    struct pi_device i2c_device;
    i2c_req_t i2c_req;
    uint32_t i2c_read_value;
    int is_awake;
} himax_impl_t;

void himax_start(himax_t *himax) {
    // GAP SDK 3.8+ also changes HIMAX_MODE_SELECT inside PI_CAMERA_CMD_START, find a better way to bypass it
    himax_impl_t *impl = (himax_impl_t *)himax->camera.data;
    pi_cpi_control_start(&impl->cpi_device);

    if (himax->current_mode == HIMAX_MODE_STANDBY) {
        himax_set_mode(himax, HIMAX_MODE_STREAMING);
    }
}

void himax_stop(himax_t *himax) {
    // GAP SDK 3.8+ also changes HIMAX_MODE_SELECT inside PI_CAMERA_CMD_STOP, find a better way to bypass it
    himax_impl_t *impl = (himax_impl_t *)himax->camera.data;
    pi_cpi_control_stop(&impl->cpi_device);
}

void himax_capture_async(himax_t *himax, frame_t *frame, pi_task_t *done_task) {
    pi_camera_capture_async(&himax->camera, frame->buffer, frame->buffer_size, done_task);
}
