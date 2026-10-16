/*
 * himax_defs.h
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

#ifndef __HIMAX_DEFS_H__
#define __HIMAX_DEFS_H__

/****************************** SENSOR ID *****************************/
#define         HIMAX_MODEL_ID_H                    0x0000  // def: 0x01
#define         HIMAX_MODEL_ID_L                    0x0001  // def: 0xB0
#define         HIMAX_SILICON_REV                   0x0002  // def: ----
#define         HIMAX_FRAME_COUNT                   0x0005  // def: 0xFF
#define         HIMAX_PIXEL_ORDER                   0x0006  // def: 0x02
/**********************************************************************/

/************************* SENSOR MODE CONTROL ************************/
#define         HIMAX_MODE_SELECT                   0x0100  // def: 0x00
#define         HIMAX_IMG_ORIENTATION               0x0101  // def: 0x00
#define         HIMAX_SW_RESET                      0x0103  // def: 0xFF
#define         HIMAX_GRP_PARAM_HOLD                0x0104  // def: 0xFF
/**********************************************************************/

/******************** SENSOR EXPOSURE GAIN CONTROL ********************/
#define         HIMAX_INTEGRATION_H                 0x0202  // def: 0x01
#define         HIMAX_INTEGRATION_L                 0x0203  // def: 0x08
#define         HIMAX_ANALOG_GAIN                   0x0205  // def: 0x00
#define         HIMAX_DIGITAL_GAIN_H                0x020E  // def: 0x01
#define         HIMAX_DIGITAL_GAIN_L                0x020F  // def: 0x00
/**********************************************************************/

/************************ FRAME TIMING CONTROL ************************/
#define         HIMAX_FRAME_LEN_LINES_H             0x0340  // def: 0x02
#define         HIMAX_FRAME_LEN_LINES_L             0x0341  // def: 0x32
#define         HIMAX_LINE_LEN_PCK_H                0x0342  // def: 0x01
#define         HIMAX_LINE_LEN_PCK_L                0x0343  // def: 0x72
/**********************************************************************/

/************************ BINNING MODE CONTROL ************************/
#define         HIMAX_READOUT_X                     0x0383  // def: 0x01
#define         HIMAX_READOUT_Y                     0x0387  // def: 0x01
#define         HIMAX_BINNING_MODE                  0x0390  // def: 0x00
/**********************************************************************/

/************************ TEST PATTERN CONTROL ************************/
#define         HIMAX_TEST_PATTERN_MODE             0x0601  // def: 0x00
/**********************************************************************/

/************************* BLACK LEVEL CONTROL ************************/
#define         HIMAX_BLC_CFG                       0x1000  // def: 0x01
#define         HIMAX_BLC_TGT                       0x1003  // def: 0x20
#define         HIMAX_BLI_EN                        0x1006  // def: 0x00
#define         HIMAX_BLC2_TGT                      0x1007  // def: 0x20
/**********************************************************************/

/*************************** SENSOR RESERVED **************************/
#define         HIMAX_DPC_CTRL                      0x1008  // def: 0x00
#define         HIMAX_SINGLE_THR_HOT                0x100B  // def: 0xFF
#define         HIMAX_SINGLE_THR_COLD               0x100C  // def: 0xFF
/**********************************************************************/

/**************** VSYNC, HSYNC AND PIXEL SHIFT REGISTER ***************/
#define         HIMAX_VSYNC_HSYNC_PIXEL_SHIFT_EN    0x1012  // def: 0x07
/**********************************************************************/

/******************* STATISTIC CONTROL AND READ ONLY ******************/
#define         HIMAX_STATISTIC_CTRL                0x2000  // def: 0x07
#define         HIMAX_MD_LROI_X_START_H             0x2011  // def: 0x00
#define         HIMAX_MD_LROI_X_START_L             0x2012  // def: 0x48
#define         HIMAX_MD_LROI_Y_START_H             0x2013  // def: 0x00
#define         HIMAX_MD_LROI_Y_START_L             0x2014  // def: 0x70
#define         HIMAX_MD_LROI_X_END_H               0x2015  // def: 0x00
#define         HIMAX_MD_LROI_X_END_L               0x2016  // def: 0xDB
#define         HIMAX_MD_LROI_Y_END_H               0x2017  // def: 0x00
#define         HIMAX_MD_LROI_Y_END_L               0x2018  // def: 0xB3
/**********************************************************************/

/******************* AUTOMATIC EXPOSURE GAIN CONTROL ******************/
#define         HIMAX_AE_CTRL                       0x2100  // def: 0x01
#define         HIMAX_AE_TARGET_MEAN                0x2101  // def: 0x3C
#define         HIMAX_AE_MIN_MEAN                   0x2102  // def: 0x0A
#define         HIMAX_CONVERGE_IN_TH                0x2103  // def: 0x03
#define         HIMAX_CONVERGE_OUT_TH               0x2104  // def: 0x05
#define         HIMAX_MAX_INTG_H                    0x2105  // def: 0x01
#define         HIMAX_MAX_INTG_L                    0x2106  // def: 0x54
#define         HIMAX_MIN_INTG                      0x2107  // def: 0x02
#define         HIMAX_MAX_AGAIN_FULL                0x2108  // def: 0x03
#define         HIMAX_MAX_AGAIN_BIN2                0x2109  // def: 0x04
#define         HIMAX_MIN_AGAIN                     0x210A  // def: 0x00
#define         HIMAX_MAX_DGAIN                     0x210B  // def: 0xC0
#define         HIMAX_MIN_DGAIN                     0x210C  // def: 0x40
#define         HIMAX_DAMPING_FACTOR                0x210D  // def: 0x20
#define         HIMAX_FS_CTRL                       0x210E  // def: 0x03
#define         HIMAX_FS_60HZ_H                     0x210F  // def: 0x00
#define         HIMAX_FS_60HZ_L                     0x2110  // def: 0x3C
#define         HIMAX_FS_50HZ_H                     0x2111  // def: 0x00
#define         HIMAX_FS_50HZ_L                     0x2112  // def: 0x32
#define         HIMAX_FS_HYST_TH                    0x2113  // def: 0x66
/**********************************************************************/

/********************** MOTION DETECTION CONTROL **********************/
#define         HIMAX_MD_CTRL                       0x2150  // def: 0x03
#define         HIMAX_I2C_CLEAR                     0x2153  // def: 0x00
#define         HIMAX_WMEAN_DIFF_TH_H               0x2155  // def: 0x7D
#define         HIMAX_WMEAN_DIFF_TH_M               0x2156  // def: 0x4B
#define         HIMAX_WMEAN_DIFF_TH_L               0x2157  // def: 0x05
#define         HIMAX_MD_THH                        0x2158  // def: 0x80
#define         HIMAX_MD_THM1                       0x2159  // def: 0x32
#define         HIMAX_MD_THM2                       0x215A  // def: 0x19
#define         HIMAX_MD_THL                        0x215B  // def: 0x03
/**********************************************************************/
     
/************************ SENSOR TIMING CONTROL ***********************/
#define         HIMAX_QVGA_WIN_EN                   0x3010  // def: 0x00
#define         HIMAX_SIX_BIT_MODE_EN               0x3011  // def: 0x70
#define         HIMAX_PMU_AUTOSLEEP_FRAMECNT        0x3020  // def: 0x00
#define         HIMAX_ADVANCE_VSYNC                 0x3022  // def: 0x02
#define         HIMAX_ADVANCE_HSYNC                 0x3023  // def: 0x02
#define         HIMAX_EARLY_GAIN                    0x3035  // def: 0xF3
/**********************************************************************/

/************************ IO AND CLOCK CONTROL ************************/
#define         HIMAX_BIT_CONTROL                   0x3059  // def: 0x02
#define         HIMAX_OSC_CLK_DIV                   0x3060  // def: 0x0A
#define         HIMAX_ANA_Register_11               0x3061  // def: 0x00
#define         HIMAX_IO_DRIVE_STR                  0x3062  // def: 0x00
#define         HIMAX_IO_DRIVE_STR2                 0x3063  // def: 0x00
#define         HIMAX_ANA_Register_14               0x3064  // def: 0x00
#define         HIMAX_OUTPUT_PIN_STATUS_CONTROL     0x3065  // def: 0x00
#define         HIMAX_ANA_Register_17               0x3067  // def: 0x00
#define         HIMAX_PCLK_POLARITY                 0x3068  // def: 0x20
/**********************************************************************/

/************************* I2C SLAVE REGISTERS ************************/
#define         HIMAX_I2C_ID_SEL                    0x3400  // def: 0x00
#define         HIMAX_I2C_ID_REG                    0x3401  // def: 0x30
/**********************************************************************/

/************************ CAMERA CLOCK SETTINGS ***********************/
// ID:     0       1       2       3
// VAL:    0x01    0x02    0x04    0x08
// DIV:    /1      /2      /4      /8
static uint8_t vt_div[4] = {1, 2, 4, 8};

// ID:     0       1       2       3
// VAL:    0x00    0x01    0x02    0x03
// DIV:    /8      /4      /2      /1
static uint8_t vt_sys_div_rev_lut[4] = {8, 4, 2, 1};

// ID:     0       1       2       3
// VAL:    0x00    0x01    0x02    0x03
// DIV:    /4      /8      /1      /2
static uint8_t vt_reg_div_rev_lut[4] = {4, 8, 1, 2};

//                 vt_sys_div
// vt_     /1/1    /1/2    /1/4    /1/8
// reg_    /2/1    /2/2    /2/4    /2/8
// div     /4/1    /4/2    /4/4    /4/8
//         /8/1    /8/2    /8/4    /8/8
static uint8_t vt_sys_reg_div_lut[4][4] = {
    {0x0B, 0x0A, 0x09, 0x08},
    {0x0F, 0x0E, 0x0D, 0x0C},
    {0x03, 0x02, 0x01, 0x00},
    {0x07, 0x06, 0x05, 0x04}
};

#endif // __HIMAX_DEFS_H__
