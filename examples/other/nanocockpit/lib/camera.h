/*
 * camera.h
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

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "config.h"
#include "coroutine.h"
#include "camera/himax.h"
#include "utils.h"

#include <pmsis.h>

#include <stdbool.h>

typedef struct frame_s {
    uint8_t *buffer;
    size_t buffer_size;

    // Whether memory for the buffer is managed by the camera
    bool managed;

    co_event_t done_event;
    co_fn_ctx_t consumer_ctx;

    // Sequential frame ID from the camera's hardware frame counter
    uint8_t frame_id;
    
    // GAP8 end-of-frame timestamp [usec]
    uint32_t frame_timestamp;
} frame_t;

typedef struct camera_s {
    himax_t himax;
    co_fn_ctx_t camera_ctx;

    frame_t frames[CAMERA_BUFFERS];

    co_fn_t consumer_callback;
} camera_t;

void camera_init(camera_t *camera, co_fn_t consumer_callback);

void camera_init_frames_alloc(camera_t *camera);
void camera_init_frames_external(camera_t *camera, int n_buffers, uint8_t *buffers[], size_t buffers_size);

size_t camera_get_buffer_size(const camera_t *camera);
int camera_get_buffer_id(const camera_t *camera, const frame_t *frame);

void camera_start(camera_t *camera);

#endif // __CAMERA_H__
