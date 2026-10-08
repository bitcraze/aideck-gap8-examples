/*
 * camera.c
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

#include "camera.h"

#include "config.h"
#include "debug.h"
#include "time.h"
#include "trace.h"

#include <pmsis.h>
#include <bsp/camera.h>

#include <stdbool.h>
#include <string.h>

CO_FN_DECLARE(camera_task);
static void camera_crop_frame_async(camera_t *camera, frame_t *frame, pi_task_t *done_task);
CO_FN_DECLARE(camera_crop_task);
static void camera_consume_frame_async(camera_t *camera, frame_t *frame, pi_task_t *done_task);

static void camera_frame_init(camera_t *camera, frame_t *frame, uint8_t *buffer, size_t buffer_size, bool managed) {
    size_t expected_buffer_size = camera_get_buffer_size(camera);

    VERBOSE_PRINT(
        "Camera buffer #%d:\t\t%s, %dB @ L2, %p, %s\n",
        camera_get_buffer_id(camera, frame),
        buffer ? "OK" : "Failed",
        buffer_size, buffer,
        managed ? "managed" : "external"
    );

    if (buffer_size != expected_buffer_size) {
        CO_ASSERTION_FAILURE("Camera buffer size mismatch (got %d but expected %d).\n", buffer_size, expected_buffer_size);
    }

    if (buffer == NULL) {
        CO_ASSERTION_FAILURE("Camera buffer allocation failed.\n");
    }

    frame->buffer = buffer;
    frame->buffer_size = buffer_size;
    frame->managed = managed;
}

static void camera_frame_free(camera_t *camera, frame_t *frame) {
    if (frame->managed) {
        pi_l2_free(frame->buffer, frame->buffer_size);
    }

    frame->buffer = NULL;
    frame->buffer_size = 0;
    frame->managed = false;
}

void camera_init(camera_t *camera, co_fn_t consumer_callback) {
    int32_t status = himax_init(&camera->himax);

    VERBOSE_PRINT("Camera init:\t\t\t%s\n", status ? "Failed" : "OK");

    if (status) {
        pmsis_exit(status);
    }

    himax_configure(&camera->himax);

    VERBOSE_PRINT(
        "Camera crop:\t\t\t%d x %dpx (TOP %dpx, LEFT %dpx, RIGHT %dpx, BOTTOM %dpx)\n",
        CAMERA_CROP_WIDTH, CAMERA_CROP_HEIGHT,
        CAMERA_CROP_TOP, CAMERA_CROP_LEFT, CAMERA_CROP_RIGHT, CAMERA_CROP_BOTTOM
    );

    camera->consumer_callback = consumer_callback;
}

void camera_init_frames_alloc(camera_t *camera) {
    size_t buffer_size = camera_get_buffer_size(camera);

    for (int i = 0; i < CAMERA_BUFFERS; i++) {
        uint8_t *buffer = pi_l2_malloc(buffer_size);
        camera_frame_init(camera, &camera->frames[i], buffer, buffer_size, /* managed */ true);
    }
}

void camera_init_frames_external(camera_t *camera, int n_buffers, uint8_t *buffers[], size_t buffers_size) {
    if (n_buffers != CAMERA_BUFFERS) {
        CO_ASSERTION_FAILURE("Number of camera buffer mismatch (got %d but expected %d).\n", n_buffers, CAMERA_BUFFERS);
    }

    for (int i = 0; i < n_buffers; i++) {
        camera_frame_init(camera, &camera->frames[i], buffers[i], buffers_size, /* managed */ false);
    }
}

size_t camera_get_buffer_size(const camera_t *camera) {
    return CAMERA_CAPTURE_HEIGHT * CAMERA_CAPTURE_WIDTH * CAMERA_CAPTURE_BPP;
}

int camera_get_buffer_id(const camera_t *camera, const frame_t *frame) {
    return frame - camera->frames;
}

void camera_start(camera_t *camera) {
    co_fn_push_start(&camera->camera_ctx, camera_task, (void *)camera, NULL);
}

CO_FN_BEGIN(camera_task, camera_t *, camera)
{
    static int capture_idx, crop_idx, consume_idx;
    static frame_t *frame;

    capture_idx = 0;
    crop_idx = 0;
    consume_idx = 0;

    while (true) {
        {
            frame = &camera->frames[capture_idx % CAMERA_BUFFERS];

            if (capture_idx >= CAMERA_BUFFERS) {
                CO_WAIT(&frame->done_event);
            }

            himax_capture_async(&camera->himax, frame, co_event_init(&frame->done_event));
            himax_start(&camera->himax);
            trace_set((capture_idx % CAMERA_BUFFERS == 0) ? TRACE_CAMERA_BUF_0 : TRACE_CAMERA_BUF_1, true);

            capture_idx += 1;
        }

        {
            frame = &camera->frames[crop_idx % CAMERA_BUFFERS];

            CO_WAIT(&frame->done_event);

            trace_set((crop_idx % CAMERA_BUFFERS == 0) ? TRACE_CAMERA_BUF_0 : TRACE_CAMERA_BUF_1, false);
            himax_stop(&camera->himax);
            frame->frame_id = himax_get_frame_count(&camera->himax);
            frame->frame_timestamp = time_get_us();

#ifdef HIMAX_CONFIG_DUMP_ONCE
            if (crop_idx == 0) {
                VERBOSE_PRINT("HIMAX config after first frame\n");
                himax_dump_config(&camera->himax);
            }
#endif

            camera_crop_frame_async(camera, frame, co_event_init(&frame->done_event));

            crop_idx += 1;
        }

        {
            frame = &camera->frames[consume_idx % CAMERA_BUFFERS];

            CO_WAIT(&frame->done_event);

            camera_consume_frame_async(camera, frame, co_event_init(&frame->done_event));

            consume_idx += 1;
        }
    }
}
CO_FN_END()

static void camera_crop_frame_async(camera_t *camera, frame_t *frame, pi_task_t *done_task) {
    co_fn_push_start(&frame->consumer_ctx, camera_crop_task, (void *)frame, done_task);
}

#define CAMERA_CROP_YIELD 1

CO_FN_BEGIN(camera_crop_task, frame_t *, frame)
{
    static uint8_t *src_buffer;
    static uint8_t *dst_buffer;
    static size_t i;

    // TODO: return immediately if data doesn't need to be moved in memory
    // * all(CAMERA_CROP_{TOP,LEFT,RIGHT} == 0), CAMERA_CROP_BOTTOM can be != 0
    // * all(CAMERA_CROP_{LEFT,RIGHT} == 0) can be handled by changing only the frame buffer pointer

    src_buffer = frame->buffer + CAMERA_CROP_TOP * CAMERA_CAPTURE_WIDTH;
    dst_buffer = frame->buffer;

    trace_set(TRACE_CAMERA_CROP, true);

    for (i = 0; i < CAMERA_CROP_HEIGHT; i++) {
        memmove(dst_buffer, src_buffer + CAMERA_CROP_LEFT, CAMERA_CROP_WIDTH * sizeof(uint8_t));
        src_buffer += CAMERA_CAPTURE_WIDTH;
        dst_buffer += CAMERA_CROP_WIDTH;

        // TODO: yield based on time instead of rows, to be independent of camera resolution 
        if (i % CAMERA_CROP_YIELD == 0) {
            trace_set(TRACE_CAMERA_CROP, false);
            CO_YIELD();
            trace_set(TRACE_CAMERA_CROP, true);
        }
    }

    trace_set(TRACE_CAMERA_CROP, false);
}
CO_FN_END()

static void camera_consume_frame_async(camera_t *camera, frame_t *frame, pi_task_t *done_task) {
    co_fn_push_start(&frame->consumer_ctx, camera->consumer_callback, (void *)frame, done_task);
}
