/*
 * config.h
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

/************************** GENERAL SETTINGS **************************/
#define VERBOSE

// Enable debug prints in coroutine.h
// #define CO_VERBOSE

// Enable debug prints in queue.c
// #define QUEUE_VERBOSE

// Enable CPX debug prints
// #define CPX_VERBOSE

// Enable CPX SPI debug prints
// #define CPX_SPI_VERBOSE

// Enable streamer debug prints
// #define STREAMER_VERBOSE

// Disable streamer: streamer_send_frame_async becomes a no-op and completes immediately
// #define STREAMER_DISABLE

/**************************** SOC SETTINGS ****************************/
#define SOC_VOLTAGE                 (1200)
#define SOC_FREQ_FC                 (246000000)
#define SOC_FREQ_CL                 (175000000)

// Target board, used to load the correct pad configurations
//  - 0: AI-deck [default]
//  - 1: GAPuino/lab
// TODO: GAPuino/GAPlab are untested in this implementation
#define BOARD                       (0)

/*************************** HIMAX SETTINGS ****************************
 *                                                                     *
 * Frame resolution selection:                                         *
 *                  ___________ ___________ ___________ ___________    *
 *    HIMAX_FORMAT |  0: FULL  |  1: QVGA  |  2: HALF  |  3: QQVGA |   *
 *                 |___________|___________|___________|___________|   *
 *      Resolution |  324x324  |  324x244  |  162x162  |  162x122  |   *
 *                 |___________|___________|___________|___________|   *
 *  Max frame rate |   45fps   |   60fps   |   90fps   |   120fps  |   *
 *                 |___________|___________|___________|___________|   *
 *                                                                     *
 * Clock mode selection:                                               *
 *  - 0: MCLK mode, 0x3067[1:0]=0x00 [default] [datasheet errata]      *
 *  - 1: OSC mode, 0x3067[1:0]=0x01                                    *
 *                                                                     *
 * Clock setup:                                                        *
 *  - OSC divider always /2 (not configurable) [datasheet errata]      *
 *  - if MCLK PAD is not driven, OSC mode is forced and the internal   *
 *    oscillator is used                                               *
 *  - the camera internal oscillator runs @ 12MHz with /2 divider      *
 *  - if MCLK PAD is driven and MCLK mode is selected, the divider can *
 *    be selected [/1,/2,/4,/8]                                        *
 *  - if MCLK PAD is driven and OSC mode is selected, MCLK is used /2  *
 *                                                                     *
 * Boards notes:                                                       *
 *  - AI-deck: it is possible to drive MCLK with a PWM timer from GAP  *
 *    (PAD: B11 - timer0_ch0). NOTE: the PWM frequency must be an      *
 *    integer divisor of the main SOC clock frequency (SOC_FREQ_FC)    *
 *  - GAP-uino/lab: the camera's MCLK pad is connected to an external  *
 *    oscillator @ 10MHz                                               *
 *                                                                     *
 * Supported clock configurations:                                     *
 *      ____________________________________________________________   *
 *     | ANA |  Mode   | GAP-uino/lab  |           AI-deck          |  *
 *     |_____|_________|_______________|____________________________|  *
 *     |  0  |  MCLK   |     10MHz     |       timer0_ch0 B11       |  *
 *     |_____|_________|_______________|____________________________|  *
 *     |     |         |               | 12MHz (if MCLK not driven) |  *
 *     |  1  |   OSC   |     10MHz     | timer0_ch0 B11 (otherwise) |  *
 *     |_____|_________|_______________|____________________________|  *
 *                                                                     *
 * Divider selection in MCLK mode:                                     *
 * - /1: 0x3060[1:0] = 0x03 [default] [datasheet errata]               *
 * - /2: 0x3060[1:0] = 0x02                                            *
 * - /4: 0x3060[1:0] = 0x01                                            *
 * - /8: 0x3060[1:0] = 0x00                                            *
 *                                                                     *
 **********************************************************************/

// Frame resolution selection
#define HIMAX_FORMAT               (2)

// Clock mode selection
//  - 0: MCLK mode [default]
//  - 1: OSC mode
#define HIMAX_ANA (0)

// Himax clock frequency (configurable only on AI-deck in MCLK mode)
#define HIMAX_FQCY (6000000)

// Himax clock dividers (configurable only in MCLK mode)
//  - 0: div /1
//  - 1: div /2
//  - 2: div /4
//  - 3: div /8
#define HIMAX_SYS_DIV (0)
#define HIMAX_REG_DIV (0)

// Himax image orientation
#define HIMAX_ORIENTATION (0x03)

// Himax auto-exposure
//  - 0: disabled
//  - 1: enabled [default]
#define HIMAX_AE (0)

//// Himax manual exposure settings ////
// Image integration time [ms]
#define HIMAX_INTEGRATION_MS (10.0f)

// Analog gain (1x to 16x), refer to datasheet for details
#define HIMAX_AGAIN          (0x10)

// Digital gain, TODO: seems to have no effect
#define HIMAX_DGAIN          (0x0100)     

// Himax desired frame rate [Hz]
#define HIMAX_FRAME_RATE    (30.0f)

// Print HIMAX configuration after acquiring the first frame (requires VERBOSE)
// #define HIMAX_CONFIG_DUMP_ONCE

// Print register values every time they are read (requires VERBOSE)
// #define HIMAX_REG_DUMP

// Read registers immediately after writing them to ensure they have
// stored the correct value
// #define HIMAX_REG_VALIDATE

/************************** CAMERA SETTINGS ***************************/

// Number of camera buffers to allocate (currently supported: 1, 2)
#define CAMERA_BUFFERS              (2)

// Crop configuration
#if HIMAX_FORMAT == 0 || HIMAX_FORMAT == 1
#define CAMERA_CROP_TOP    (2)
#define CAMERA_CROP_LEFT   (2)
#define CAMERA_CROP_RIGHT  (2)
#define CAMERA_CROP_BOTTOM (2)
#else
#define CAMERA_CROP_TOP    (1)
#define CAMERA_CROP_LEFT   (1)
#define CAMERA_CROP_RIGHT  (1)
#define CAMERA_CROP_BOTTOM (1)
#endif

/**************************** CPX SETTINGS ****************************/

// Enable bidirectional CPX SPI communication (GAP<=>ESP32).
// Disabled by default (i.e., GAP->ESP32 only), because it allows a 
// much higher SPI bandwidth compared to bidirectional communication. 
// This is further made worse by an AI-deck PCB bug.
// (see also src/nina/main/spi.c)
#define CPX_SPI_BIDIRECTIONAL

/************************* STREAMER SETTINGS **************************/

// Compute CRC32 checksum on transmitted buffers
// #define STREAMER_SEND_CHECKSUM

// Verify CRC32 checksum on received buffers
#define STREAMER_RECEIVE_CHECKSUM

/***********************************************************************
 *                                                                     *
 *          WARNING: DO NOT MODIFY THE FOLLOWING PARAMETERS            *
 *                                                                     *
 **********************************************************************/

/*************************** GPIO SETTINGS ****************************/
// Available GPIOs on AI-Deck:
//                       PMSIS function             Schematic name      Notes
#define GPIO_LED         PI_GPIO_A2_PAD_14_A2    // GAP8_LED            accessible with clip on LED, free
#define GPIO_GAP8_RTT    PI_GPIO_A3_PAD_15_B1    // GAP8_GPIO_NINA_IO   not accessible [ERRATA: 1V8 pin, use only as GAP8 out -> NINA in, DOUBLE ERRATA: according to padframe.xlsx, it's NINA_GPIO_GAP8_IO that is supposed to be 1V8 instead]
#define GPIO_NINA_RTT    PI_GPIO_A18_PAD_32_A13  // NINA_GPIO_GAP8_IO   not accessible
#define GPIO_I2C_SDA     PI_GPIO_A15_PAD_29_B34  // SPARE_I2C_SDA       accessible from header, pulled-up on cf side (also TIMER3_CH3)
#define GPIO_I2C_SCL     PI_GPIO_A16_PAD_30_D1   // SPARE_I2C_SCL       accessible from header, pulled-up on cf side
#define GPIO_TIMER0_CH0  PI_GPIO_A17_PAD_31_B11  // GAP8_TIMER0CH0_1V8  accessible with clip on U9 2, camera MCLK
#define GPIO_UART_RX     PI_GPIO_A24_PAD_38_B6   // GAP8_UART_RX_3V     accessible from header, driven by cf (usable only as input)
#define GPIO_UART_TX     PI_GPIO_A25_PAD_39_A7   // GAP8_UART_TX_1V8    accessible from header, free

/*************************** HIMAX SETTINGS ***************************/
// Certain combinations of boards and clock modes only support a specific 
// clock frequency and divider configuration.
// The following defines will cause a compilation error if you try to 
// change a parameter that cannot be changed.
#if BOARD == 0 // AI-deck
    #if HIMAX_ANA == 0 // MCLK mode
        // HIMAX_FQCY, HIMAX_SYS_DIV, and HIMAX_REG_DIV are fully configurable
        
        #if SOC_FREQ_FC % HIMAX_FQCY != 0
            #error Desired HIMAX_FQCY cannot be generated from the current SOC_FREQ_FC, \
                   you should change SOC_FREQ_FC to be a multiple of HIMAX_FQCY
        #endif
	#elif HIMAX_ANA == 1 // OSC mode
        // FIXME: this might be incorrect, from the datasheet it looks like 48MHz /8
        #define HIMAX_FQCY 12000000     // 12MHz internal osc
 		#define HIMAX_SYS_DIV 1         // div /2
 		#define HIMAX_REG_DIV 1         // div /2
	#endif
#elif BOARD == 1 // GAPuino/lab
    #if HIMAX_ANA == 0 // MCLK mode
        #define HIMAX_FQCY 10000000     // 10MHz external osc
	#elif HIMAX_ANA == 1 // OSC mode
        #define HIMAX_FQCY 10000000     // 10MHz external osc
 		#define HIMAX_SYS_DIV 1         // div /2
  		#define HIMAX_REG_DIV 1         // div /2
    #endif

    // Remap RTT pins to accessible GPIOs
    #undef GPIO_NINA_RTT
    #undef GPIO_GAP8_RTT
    #define GPIO_NINA_RTT PI_GPIO_A17_PAD_31_B11
    #define GPIO_GAP8_RTT PI_GPIO_A4_PAD_16_A44
#endif

#if HIMAX_FORMAT == 0
    #define HIMAX_WIDTH   (324)
    #define HIMAX_HEIGHT  (324)
#elif HIMAX_FORMAT == 1
    #define HIMAX_WIDTH   (324)
    #define HIMAX_HEIGHT  (244)
#elif HIMAX_FORMAT == 2
    #define HIMAX_WIDTH   (162)
    #define HIMAX_HEIGHT  (162)
#elif HIMAX_FORMAT == 3
    #define HIMAX_WIDTH   (162)
    #define HIMAX_HEIGHT  (122)
#endif

#define HIMAX_BPP  (1)

/************************** CAMERA SETTINGS ***************************/

#define CAMERA_CAPTURE_WIDTH   (HIMAX_WIDTH)
#define CAMERA_CAPTURE_HEIGHT  (HIMAX_HEIGHT - CAMERA_CROP_BOTTOM)
#define CAMERA_CAPTURE_BPP     (HIMAX_BPP)

#define CAMERA_CROP_WIDTH      (CAMERA_CAPTURE_WIDTH - CAMERA_CROP_LEFT - CAMERA_CROP_RIGHT)
#define CAMERA_CROP_HEIGHT     (CAMERA_CAPTURE_HEIGHT - CAMERA_CROP_TOP)
#define CAMERA_CROP_BPP        (1)

#endif // __CONFIG_H__
