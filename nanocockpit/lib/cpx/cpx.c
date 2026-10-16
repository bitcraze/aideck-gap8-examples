/*
 * cpx.c
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

#include "cpx.h"

#include "config.h"
#include "debug.h"
#include "trace.h"

#include <pmsis.h>

#include <stdint.h>

#ifdef CPX_VERBOSE
    #define CPX_VERBOSE_PRINT(...) CO_PRINT(__VA_ARGS__)
#else
    #define CPX_VERBOSE_PRINT(...) 
#endif

CO_FN_DECLARE(cpx_send_task);
CO_FN_DECLARE(cpx_receive_task);

cpx_send_req_t *cpx_send_req_alloc(uint16_t payload_capacity) {
    cpx_send_req_t *req = pi_l2_malloc(sizeof(cpx_send_req_t) + payload_capacity);
    
    if (!req) {
        CO_ASSERTION_FAILURE("Could not alloc cpx_send_req_t.\n");
    }
    
    *req = (cpx_send_req_t){0};

    req->payload_capacity = payload_capacity;
    
    cpx_spi_send_req_init(
        &req->req,
        req->payload, payload_capacity, // Head
        NULL, 0                         // Tail
    );

#ifdef CPX_VERBOSE    
    memset(req->payload, 0x88, payload_capacity);
#endif

    return req;
}

void cpx_send_req_set_head_length(cpx_send_req_t *req, uint16_t payload_length) {
    if (payload_length > req->payload_capacity) {
        CO_ASSERTION_FAILURE("CPX payload length (%d) exceeds allocated capacity (%d)", payload_length, req->payload_capacity);
    }

    cpx_spi_send_set_head(&req->req, req->payload, payload_length);
}

uint16_t cpx_send_req_max_tail_length(cpx_send_req_t *req) {
    return cpx_spi_send_max_tail_length(&req->req);
}

void cpx_send_req_set_tail(cpx_send_req_t *req, uint8_t *payload_tail, uint16_t tail_length) {
    cpx_spi_send_set_tail(&req->req, payload_tail, tail_length);
}

void cpx_init(cpx_t *cpx) {
    cpx_spi_init(&cpx->cpx_spi);

    // Allocate memory for received packets (memory for sent packets is provided by the sender)
    cpx_spi_receive_req_init(&cpx->receive_req.req);

    for (int i = 0; i < CPX_F_LAST; i++) {
        cpx->receive_callbacks[i] = NULL;
        cpx->receiver_args[i] = NULL;
    }

    VERBOSE_PRINT("CPX init:\t\t\tOK\n");
}

void cpx_register_rx_callback(cpx_t *cpx, cpx_function_e function, co_fn_t receive_callback, void *receiver_args) {
    if (cpx->receive_callbacks[function] != NULL) {
        CO_ASSERTION_FAILURE("CPX function %d already has a registered receive callback (%p)", function, receive_callback);
    }

    cpx->receive_callbacks[function] = receive_callback;
    cpx->receiver_args[function] = receiver_args;
}

void cpx_start(cpx_t *cpx) {
    cpx_spi_start(&cpx->cpx_spi);

    // send_done is used to serialize multiple send requests, mark the event as completed initially
    // TODO: provide an actual API to do this (es. a mutex)
    pi_task_push(co_event_init(&cpx->send_done));

    co_fn_push_start(&cpx->receive_ctx, cpx_receive_task, (void *)cpx, NULL);
}

void cpx_send_async(cpx_t *cpx, cpx_send_req_t *send_req, pi_task_t *done_task) {
    send_req->cpx = cpx;

    co_fn_push_start(&send_req->ctx, cpx_send_task, (void *)send_req, done_task);
}

CO_FN_BEGIN(cpx_send_task, cpx_send_req_t *, send_req)
{
    trace_set(TRACE_CPX_SEND, true);

    // FIXME: this probably deadlocks if contended
    // Wait until all previous sends have completed
    // This works because the scheduler guarantees that tasks wake up in same the order they 
    // started waiting on send_done
    while (!co_event_is_done(&send_req->cpx->send_done)) {
        CO_WAIT(&send_req->cpx->send_done);
    }

    // FIXME: design a proper API to set this
    send_req->req.header.cpx = send_req->header;

    cpx_spi_send_async(&send_req->cpx->cpx_spi, &send_req->req, co_event_init(&send_req->cpx->send_done));
    CO_WAIT(&send_req->cpx->send_done);

    CPX_VERBOSE_PRINT("Sent packet with size %d bytes:\n", send_req->req.header.length);
    CPX_VERBOSE_PRINT("HEAD: ");
    for (size_t i = 0; i < send_req->req.head_length; i++) {
        CPX_VERBOSE_PRINT("%02x ", send_req->req.payload_head[i]);
    }
    CPX_VERBOSE_PRINT("\n");
    CPX_VERBOSE_PRINT("TAIL: ");
    for (size_t i = 0; i < send_req->req.tail_length; i++) {
        CPX_VERBOSE_PRINT("%02x ", send_req->req.payload_tail[i]);
    }
    CPX_VERBOSE_PRINT("\n");

    trace_set(TRACE_CPX_SEND, false);
}
CO_FN_END()

static void cpx_dispatch_callback_async(cpx_t *cpx, cpx_receive_req_t *req, pi_task_t *done_task) {
    co_fn_t receive_callback = NULL;
    
    if (req->header->function < CPX_F_LAST) {
        receive_callback = cpx->receive_callbacks[req->header->function];
    }

    if (receive_callback) {
        req->receiver_args = cpx->receiver_args[req->header->function];;
        co_fn_push_start(&cpx->callback_ctx, receive_callback, (void *)req, done_task);
    } else {
        // No receiver configured for this function, drop the message
        pi_task_push(done_task);
    }
}

CO_FN_BEGIN(cpx_receive_task, cpx_t *, cpx)
{
    while (true) {
        trace_set(TRACE_CPX_RECEIVE, true);

        cpx_spi_receive_async(&cpx->cpx_spi, &cpx->receive_req.req, co_event_init(&cpx->receive_done));
        CO_WAIT(&cpx->receive_done);

        uint8_t *receive_buffer = cpx->receive_req.req.buffer;
        uint16_t receive_length = cpx->receive_req.req.header.length;

        CPX_VERBOSE_PRINT("Received packet with size %d bytes\n", receive_length);
        if (receive_length > 0) {
            for (size_t i = 0; i < CPX_SPI_MTU; i++) {
                CPX_VERBOSE_PRINT("%02x ", receive_buffer[i]);
            }
            CPX_VERBOSE_PRINT("\n");

            trace_set(TRACE_CPX_RECEIVE, false);

            cpx_header_t *cpx_header = &cpx->receive_req.req.header.cpx;
            if (cpx_header->version != CPX_VERSION) {
                CO_ASSERTION_FAILURE("Received packet with unsupported CPX version %d, expected %d.\n", cpx_header->version, CPX_VERSION);
            }

            cpx->receive_req.header = cpx_header;
            cpx->receive_req.payload = receive_buffer;
            cpx->receive_req.payload_length = receive_length;

            cpx_dispatch_callback_async(cpx, &cpx->receive_req, co_event_init(&cpx->receive_done));
            CO_WAIT(&cpx->receive_done);
        }

        // for (size_t i = 0; i < CPX_SPI_MTU; i++) {
        //     cpx->packets[0].buffer[i] = 0x77;
        // }

        // for (size_t i = 0; i < CPX_SPI_MTU; i++) {
        //     cpx->packets[1].buffer[i] = 0xbc;
        // }

        // cpx_spi_receive_async(&cpx->cpx_spi, &cpx->packets[0], co_event_init(&cpx->packet_done[0]));
        
        // cpx->packets[1].length = TEST_LENGTH;
        // for (size_t i = 0; i < TEST_LENGTH; i++) {
        //     cpx->packets[1].buffer[i] = i;
        // }
        // cpx_spi_send_async(&cpx->cpx_spi, &cpx->packets[1], co_event_init(&cpx->packet_done[1]));
        
        // CO_WAIT(&cpx->packet_done[0]);
        // CO_WAIT(&cpx->packet_done[1]);

        // VERBOSE_PRINT("Received packet with size %d bytes:\n", cpx->packets[0].length);
        // for (size_t i = 0; i < CPX_SPI_MTU; i++) {
        //     VERBOSE_PRINT("%d ", cpx->packets[0].buffer[i]);
        // }
        // VERBOSE_PRINT("\n");

        // VERBOSE_PRINT("Sent packet with size %d bytes\n", cpx->packets[1].length);

        // pi_task_push_delayed_us(co_event_init(&cpx->packet_done[0]), 1000000);
        // CO_WAIT(&cpx->packet_done[0]);
    }
}
CO_FN_END()
