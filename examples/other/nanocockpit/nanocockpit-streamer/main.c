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
#include "trace.h"
#include "uart.h"
#include "uart_protocol.h"

#include <pmsis.h>

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

static PI_FC_L1 co_fn_ctx_t streamer_rx_ctx;

CO_FN_BEGIN(camera_callback, frame_t *, camera_frame)
{
    static PI_FC_L1 bool camera_started = false;
    static PI_FC_L1 co_event_t streamer_tx_done;

    if (camera_started) {
        while (!co_event_is_done(&streamer_tx_done)) {
            CO_WAIT(&streamer_tx_done);
        }
    }

    camera_started = true;

    streamer_send_frame_async(
        &streamer,
        camera_frame,
        &latest_state, state_timestamp,
        &latest_tof, tof_timestamp,
        &latest_inference,
        co_event_init(&streamer_tx_done)
    );
    CO_WAIT(&streamer_tx_done);
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

        // Handle packets received from the host over Wi-Fi

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
