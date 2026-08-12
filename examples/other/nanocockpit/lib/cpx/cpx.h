/*
 * cpx.h
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

#ifndef __CPX_H__
#define __CPX_H__

#include "cpx_types.h"
#include "cpx_spi.h"
#include "coroutine.h"

#include <stdint.h>

typedef struct cpx_s cpx_t;

typedef struct cpx_send_req_s {
    cpx_header_t header;
    uint16_t payload_capacity;

    cpx_spi_send_req_t req;
    cpx_t *cpx;
    co_fn_ctx_t ctx;

    uint8_t payload[];
} cpx_send_req_t;

cpx_send_req_t *cpx_send_req_alloc(uint16_t payload_capacity);
void cpx_send_req_set_head_length(cpx_send_req_t *req, uint16_t payload_length);
uint16_t cpx_send_req_max_tail_length(cpx_send_req_t *req);
void cpx_send_req_set_tail(cpx_send_req_t *req, uint8_t *payload_tail, uint16_t tail_length);

// TODO: this is not really a "request", this represents an already received packet
typedef struct cpx_receive_req_s {
    cpx_spi_receive_req_t req;

    cpx_header_t *header;
    uint8_t *payload;
    uint16_t payload_length;

    void *receiver_args;
} cpx_receive_req_t;

typedef struct cpx_s {
    cpx_spi_t cpx_spi;

    co_event_t send_done;

    co_fn_ctx_t receive_ctx;
    cpx_receive_req_t receive_req;
    co_event_t receive_done;

    co_fn_t receive_callbacks[CPX_F_LAST];
    void *receiver_args[CPX_F_LAST];
    co_fn_ctx_t callback_ctx;
} cpx_t;

void cpx_init(cpx_t *cpx);

// Register a callback for received messages with the given function
void cpx_register_rx_callback(cpx_t *cpx, cpx_function_e function, co_fn_t receive_callback, void *receiver_args);

// Start CPX receiver task
void cpx_start(cpx_t *cpx);

void cpx_send_async(cpx_t *cpx, cpx_send_req_t *send_req, pi_task_t *done_task);

#endif // __CPX_H__
