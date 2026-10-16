/*
 * main.c
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

#include "config.h"
#include "coroutine.h"
#include "camera.h"
#include "cluster.h"
#include "cpx/cpx.h"
#include "debug.h"
#include "rng.h"
#include "soc.h"
#include "streamer.h"
#include "time.h"
#include "trace.h"
#include "queue.h"
#include "uart.h"
#include "uart_protocol.h"

#include "mem.h"
#include "network.h"

#include "pulp_nn_utils.h"

#include <pmsis.h>
#include <bsp/ram.h>

#include <math.h>
#include <stdbool.h>

static uart_t uart;
static uart_protocol_t uart_protocol;
static camera_t camera;
static cpx_t cpx;
static streamer_t streamer;
static pi_device_t cluster;

static PI_FC_L1 state_msg_t latest_state;
static PI_FC_L1 uint32_t state_timestamp;

static PI_FC_L1 tof_msg_t latest_tof;
static PI_FC_L1 uint32_t tof_timestamp;

static PI_L2 inference_stamped_msg_t latest_inference;

static void *l2_buffer;
static size_t l2_buffer_size;

typedef struct inference_args_s {
    uint32_t stm32_timestamp;
    frame_t *camera_frame;
    pi_task_t *frame_done;
} inference_args_t;

static pi_device_t *ram;
static void *test_input_l3;

static PI_FC_L1 co_fn_ctx_t inference_ctx;
static PI_FC_L1 co_fn_ctx_t streamer_rx_ctx;

CO_FN_DECLARE(inference_task);

CO_FN_BEGIN(camera_callback, frame_t *, camera_frame)
{
    static PI_FC_L1 bool camera_started = false;
    static PI_FC_L1 inference_args_t inference_args;
    static PI_FC_L1 co_event_t frame_done;
    static PI_FC_L1 co_event_t inference_done;
    static PI_FC_L1 co_event_t streamer_tx_done;

    // Load static test image for debugging
    // pi_ram_read_async(ram, (uint32_t)test_input_l3, camera_frame->buffer, NETWORK_INPUT_SIZE, co_event_init(&streamer_tx_done));
    // CO_WAIT(&streamer_tx_done);

    if (camera_started) {
#ifdef NETWORK_ONBOARD_INFERENCE
        // Wait until previous inference has completed before starting a new one
        while (!co_event_is_done(&inference_done)) {
            CO_WAIT(&inference_done);
        }
#endif

        while (!co_event_is_done(&streamer_tx_done)) {
            CO_WAIT(&streamer_tx_done);
        }
    }

    camera_started = true;

#ifdef NETWORK_ONBOARD_INFERENCE
    inference_args = (inference_args_t){
        .stm32_timestamp = latest_state.timestamp,
        .camera_frame = camera_frame,
        .frame_done = co_event_init(&frame_done)
    };
    co_fn_push_start(&inference_ctx, inference_task, &inference_args, co_event_init(&inference_done));
#endif

    streamer_send_frame_async(
        &streamer,
        camera_frame,
        &latest_state, state_timestamp,
        &latest_tof, tof_timestamp,
        &latest_inference,
        co_event_init(&streamer_tx_done)
    );
    CO_WAIT(&streamer_tx_done);

#ifdef NETWORK_ONBOARD_INFERENCE
    CO_WAIT(&frame_done);
#endif
}
CO_FN_END()

CO_FN_BEGIN(inference_task, inference_args_t *, inference_args)
{
    static PI_FC_L1 frame_t *camera_frame;
    static PI_FC_L1 pi_task_t *frame_done;
    static PI_FC_L1 co_event_t network_done;
    static PI_FC_L1 float network_output[NETWORK_OUTPUT_COUNT];

    camera_frame = inference_args->camera_frame;
    frame_done = inference_args->frame_done;

    trace_set(TRACE_USER_0, true);
    network_run_async(camera_frame->buffer, l2_buffer, l2_buffer, l2_buffer_size, 0, &cluster, frame_done, co_event_init(&network_done));
    CO_WAIT(&network_done);
    
    network_dequantize_output(l2_buffer, network_output);
    trace_set(TRACE_USER_0, false);

    latest_inference = (inference_stamped_msg_t) {
        .stm32_timestamp = inference_args->stm32_timestamp,
        .x = network_output[0],
        .y = network_output[1],
        .z = network_output[2],
        .phi = network_output[3],
    };

    uart_protocol_send_inference_async(&uart_protocol, &latest_inference, co_event_init(&network_done));
    CO_WAIT(&network_done);
}
CO_FN_END()

CO_FN_DECLARE(streamer_rx_task);

static void streamer_rx_start() {
    co_fn_push_start(&streamer_rx_ctx, streamer_rx_task, NULL, NULL);
}

CO_FN_BEGIN(streamer_rx_task, void *, arg)
{
    static PI_L2    offboard_buffer_t offboard_buffer;
    static PI_FC_L1 streamer_buffer_t offboard_buffer_rx;
    static PI_FC_L1 co_event_t done_task;

    while (true) {
        streamer_buffer_init(&offboard_buffer_rx, &offboard_buffer, sizeof(offboard_buffer));
        streamer_receive_buffer_async(&streamer, &offboard_buffer_rx, co_event_init(&done_task));
        CO_WAIT(&done_task);

        if (offboard_buffer_rx.type != STREAMER_TYPE_INFERENCE) {
            printf("discarded streamer buffer type %d (expected %d)\n", offboard_buffer_rx.type, STREAMER_TYPE_INFERENCE);
            continue;
        }

#ifndef NETWORK_ONBOARD_INFERENCE
        if (offboard_buffer.inference_stamped.stm32_timestamp != 0) {
            uart_protocol_send_inference_async(&uart_protocol, &offboard_buffer.inference_stamped, co_event_init(&done_task));
            CO_WAIT(&done_task);
        }
#endif

        streamer_stats_frame_completed(&streamer, &offboard_buffer.stats);
    }
}
CO_FN_END()

CO_FN_BEGIN(uart_callback, uart_msg_t *, message)
{
    if (memcmp(message->header, UART_STATE_MSG_HEADER, UART_HEADER_LENGTH) == 0) {
        latest_state = message->state;
        state_timestamp = message->recv_timestamp;
    } else if (memcmp(message->header, UART_RNG_MSG_HEADER, UART_HEADER_LENGTH) == 0) {
        rng_push_entropy(message->rng.entropy);
    } else if (memcmp(message->header, UART_TOF_MSG_HEADER, UART_HEADER_LENGTH) == 0) {
        latest_tof = message->tof;
        tof_timestamp = message->recv_timestamp;
    }
}
CO_FN_END()

static void main_task(void) {
    soc_init();

    uart_init(&uart);
    uart_protocol_init(&uart_protocol, &uart, uart_callback);
    
    camera_init(&camera, camera_callback);
    
    cpx_init(&cpx);

    streamer_init(&streamer, &camera, &cpx);
    streamer_alloc_frames(&streamer, &camera);

    cluster_init(&cluster);

#ifdef NETWORK_ONBOARD_INFERENCE
    mem_init();
    network_init();
    
    memory_dump(&cluster);

    ram = get_ram_ptr();
    test_input_l3 = ram_malloc(NETWORK_INPUT_SIZE);
    load_file_to_ram(test_input_l3, "inputs.hex");

    l2_buffer_size = NETWORK_L2_BUFFER_SIZE;
    l2_buffer = pi_l2_malloc(l2_buffer_size);
    VERBOSE_PRINT("Network:\t\t\t%s, %dB @ L2, 0x%08x\n", l2_buffer?"OK":"Failed", l2_buffer_size, l2_buffer);
    if (!l2_buffer) {
        pmsis_exit(-1);
    }
#endif

    memory_dump(&cluster);

    // Needs to be done last when using UART TX as a trace GPIO, to override
    // the UART configuration which happens somewhere in the SDK.
    trace_init();

    VERBOSE_PRINT("\n\t *** Initialization done ***\n\n");

    uart_protocol_start(&uart_protocol);
    camera_start(&camera);
    cpx_start(&cpx);

    streamer_rx_start();

    while (true) {
        pi_yield();
    }

    pmsis_exit(0);
}

int main(void) {
    VERBOSE_PRINT("\n\n\t *** PMSIS Kickoff ***\n\n");

    return pmsis_kickoff((void *)main_task);
}
