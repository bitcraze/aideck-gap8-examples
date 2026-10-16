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

#ifndef __CAMERA_HIMAX_H__
#define __CAMERA_HIMAX_H__

#include "config.h"

#include <pmsis.h>

#include <stdint.h>

typedef struct frame_s frame_t;

typedef enum {
    HIMAX_FULL = 0,
    HIMAX_QVGA = 1,
    HIMAX_HALF = 2,
    HIMAX_QQVGA = 3,
} himax_format_e;

typedef enum {
    HIMAX_MODE_UNKNOWN    =  -1,
    HIMAX_MODE_STANDBY    = 0x0,
    HIMAX_MODE_STREAMING  = 0x1, // Continous I2C-triggered streaming
    HIMAX_MODE_STREAMING2 = 0x2, // Output a fixed number of frames 
    HIMAX_MODE_STREAMING3 = 0x3, // Hardware-triggered streaming
} himax_mode_e;

typedef struct himax_s {
    pi_device_t camera;
    
#if HIMAX_ANA == 0 // MCLK mode
    pi_device_t mclk_timer;
#endif

    himax_mode_e current_mode;
} himax_t;

int32_t himax_init(himax_t *himax);
void himax_configure(himax_t *himax);

void himax_set_mode(himax_t *himax, himax_mode_e mode);
uint8_t himax_get_frame_count(himax_t *himax);
void himax_dump_config(himax_t *himax);

void himax_start(himax_t *himax);
void himax_stop(himax_t *himax);
void himax_capture_async(himax_t *himax, frame_t *frame, pi_task_t *done_task);

#endif // __CAMERA_HIMAX_H__
