/*
 * streamer.h
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

#ifndef __STREAMER_H__
#define __STREAMER_H__

#include "uart_protocol.h"

#include <pmsis.h>

typedef enum {
    STREAMER_TYPE_IMAGE         = 0x01,
    STREAMER_TYPE_INFERENCE     = 0xF0,
    STREAMER_TYPE_FOG_BUFFER    = 0xF1,
} __attribute__((packed)) streamer_type_e;

typedef enum {
    STREAMER_FORMAT_GRAY_8 = 0
} __attribute__((packed)) streamer_format_e;

#define STREAMER_METADATA_VERSION 10
typedef struct streamer_metadata_s {
    // Metadata format version, always equal to STREAMER_METADATA_VERSION
    uint8_t metadata_version;

    // Frame resolution and pixel format
    uint16_t frame_width;
    uint16_t frame_height;
    uint8_t frame_bpp;
    uint8_t frame_format;

    // Sequential frame ID from the camera's hardware frame counter
    uint8_t frame_id;

    // GAP8 end-of-frame timestamp [usec]
    uint32_t frame_timestamp;

    // GAP8 state received timestamp [usec]
    uint32_t state_timestamp;

    // Latest state estimation received from STM32
    state_msg_t state;

    // GAP8 TOF received timestamp [usec]
    uint32_t tof_timestamp;

    // Latest TOF frame received from STM32
    tof_msg_t tof;

    // GAP8 timestamps used for RTT latency computation [usec]
    uint32_t reply_frame_timestamp;
    uint32_t reply_recv_timestamp;

    // Latest inference computed onboard by GAP
    inference_stamped_msg_t inference;
} __attribute__((packed)) streamer_metadata_t;

typedef struct streamer_stats_s {
    // GAP8 timestamp for latest frame received by the client [usec]
    uint32_t reply_frame_timestamp;
    uint8_t  reply_frame_id;
} __attribute__((packed)) streamer_stats_t;

typedef struct offboard_buffer_s {
    // Frame statistics used by the streamer to compute the round-trip time
    streamer_stats_t stats;
    inference_stamped_msg_t inference_stamped;
} __attribute__((packed)) offboard_buffer_t;

typedef struct streamer_payload_s {
    streamer_metadata_t metadata;
    uint8_t buffer[];
} __attribute__((packed)) streamer_payload_t;

typedef struct streamer_s streamer_t;

typedef struct streamer_frame_s {
    streamer_payload_t *payload;
    size_t payload_size;

    co_fn_ctx_t send_ctx;
    streamer_t *streamer;
} streamer_frame_t;

typedef struct streamer_buffer_s {
    void *storage;
    size_t storage_capacity;

    streamer_type_e type;
    size_t size;
    size_t received_size;
    uint32_t checksum;
    
    pi_task_t *done_task;
} streamer_buffer_t;

void streamer_buffer_init(streamer_buffer_t *buffer, void *storage, size_t storage_capacity);

typedef struct camera_s camera_t;
typedef struct frame_s frame_t;
typedef struct cpx_s cpx_t;
typedef struct cpx_send_req_s cpx_send_req_t;

typedef struct streamer_s {
    camera_t *camera;

    cpx_t *cpx;
    cpx_send_req_t *cpx_req;
    co_event_t cpx_done;
    
    streamer_frame_t frames[CAMERA_BUFFERS];

    streamer_buffer_t *buffer_rx;

    uint32_t reply_frame_timestamp;
    uint32_t reply_recv_timestamp;
} streamer_t;

void streamer_init(streamer_t *streamer, camera_t *camera, cpx_t *cpx);
void streamer_alloc_frames(streamer_t *streamer, camera_t *camera);
void streamer_send_frame_async(
    streamer_t *streamer,
    frame_t *frame,
    state_msg_t *state, uint32_t state_timestamp,
    tof_msg_t *tof, uint32_t tof_timestamp,
    inference_stamped_msg_t *inference,
    pi_task_t *done_task
);

void streamer_receive_buffer_async(streamer_t *streamer, streamer_buffer_t *buffer, pi_task_t *done_task);
void streamer_cancel_receive(streamer_t *streamer, streamer_buffer_t *buffer);

// Mark the frame as completed and store the statistics to compute the round-trip time
void streamer_stats_frame_completed(streamer_t *streamer, streamer_stats_t *stats);

#endif // __STREAMER_H__
