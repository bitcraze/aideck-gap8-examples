/*
 * main.c
 * Elia Cereda <elia.cereda@idsia.ch>
 *
 * Copyright (C) 2023-2025 IDSIA, USI-SUPSI
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
#include "cpx/cpx.h"
#include "trace.h"

#include <pmsis.h>

typedef enum {
    EXAMPLE_CMD_1           = 0x01,
    EXAMPLE_CMD_2           = 0x02,
} __attribute__((packed)) example_cmd_e;

typedef struct example_cmd1_s {
    uint32_t a;
    uint8_t _padding;
} __attribute__((packed)) example_cmd1_t;

typedef struct example_cmd2_s {
    float b;
    uint8_t _padding;
} __attribute__((packed)) example_cmd2_t;

typedef struct example_packet_s {
    example_cmd_e command;
        union {
        example_cmd1_t cmd1;
        example_cmd2_t cmd2;
    };
} __attribute__((packed)) example_packet_t;

static example_packet_t *example_packet_init(cpx_send_req_t *cpx_req, example_cmd_e command) {
    cpx_send_req_set_head_length(cpx_req, sizeof(example_packet_t));
    
    example_packet_t *packet = (example_packet_t *)cpx_req->payload;
    packet->command = command;
    return packet;
}

static cpx_t cpx;

CO_FN_DECLARE(cpx_tx_task);

static co_fn_ctx_t cpx_tx_ctx;
static cpx_send_req_t *cpx_req;

void cpx_tx_init() {
    cpx_req = cpx_send_req_alloc(sizeof(example_packet_t));
    cpx_req->header = CPX_HEADER_INIT(CPX_T_WIFI_HOST, CPX_F_APP);
}

void cpx_tx_start() {
    co_fn_push_start(&cpx_tx_ctx, cpx_tx_task, NULL, NULL);
}

CO_FN_BEGIN(cpx_tx_task, void *, arg)
{
    static co_event_t cpx_done;
    static example_packet_t *packet;

    printf("CPX TX started\n");

    while (true) {
        packet = example_packet_init(cpx_req, EXAMPLE_CMD_1);
        packet->cmd1 = (example_cmd1_t){
            .a = 1234
        };
        cpx_send_async(&cpx, cpx_req, co_event_init(&cpx_done));
        CO_WAIT(&cpx_done);
        printf("Sent CPX packet cmd1\n");

        pi_task_push_delayed_us(co_event_init(&cpx_done), 500000);
        CO_WAIT(&cpx_done);

        packet = example_packet_init(cpx_req, EXAMPLE_CMD_2);
        packet->cmd2 = (example_cmd2_t){
            .b = 5678.0f
        };
        cpx_send_async(&cpx, cpx_req, co_event_init(&cpx_done));
        CO_WAIT(&cpx_done);
        printf("Sent CPX packet cmd2\n");
    }
}
CO_FN_END()

void main_task() {
    cpx_init(&cpx);
    cpx_tx_init();    

    trace_init();

    cpx_start(&cpx);
    cpx_tx_start();

    while (true) {
        pi_yield();
    }

    pmsis_exit(0);    
}

int main(void) {
    printf("\n\n\t *** PMSIS Kickoff ***\n\n");
    return pmsis_kickoff((void *)main_task);
}
