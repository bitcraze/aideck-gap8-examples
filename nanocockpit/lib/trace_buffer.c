/*
 * trace_buffer.c
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

#include "trace_buffer.h"

#include "config.h"
#include "debug.h"

#include <pmsis.h>

trace_buffer_t *trace_buffers[TRACE_NUM_CORES] = {};

void trace_buffer_init(int core_id, pi_device_t *cluster) {
    if (trace_buffers[core_id] != NULL) {
        ASSERTION_FAILURE("Trace buffer for core %d already initialized\n", core_id);
    }
    
    trace_buffer_t *buffer = NULL;
    
    if (core_id == PI_FC_CORE_ID) {
        buffer = pi_fc_l1_malloc(sizeof(trace_buffer_t));
    } else {
        if (cluster == NULL) {
            ASSERTION_FAILURE("Cluster device required to allocate buffer for core %d\n", core_id);
        }
        buffer = pi_cl_l1_malloc(cluster, sizeof(trace_buffer_t));
    }

    if (buffer == NULL) {
        ASSERTION_FAILURE("Failed to allocate memory for trace buffer for core %d\n", core_id);
    }

    memset(buffer, 0xaa, sizeof(trace_buffer_t));
    buffer->started = false;
    buffer->next_event = 0;
    buffer->event_count = 0;

    trace_buffers[core_id] = buffer;
}

void trace_buffer_start() {
    int core_id = pi_core_id();
    trace_buffer_t *t = trace_buffers[core_id];
    if (t == NULL) {
        ASSERTION_FAILURE("Trace buffer for core %d not initialized\n", core_id);
    }

    if (!t->started) {
        pi_perf_conf(1 << TRACE_EVENTS_PERF_COUNTER);
        pi_perf_reset();
        pi_perf_stop();
        pi_perf_start();

        t->started = true;
    }

    trace_sync();
}

void trace_buffer_dump_core(int core_id) {
    trace_buffer_t *t = trace_buffers[core_id];
    
    if (t == NULL) {
        return;
    }

    int first_event = (t->next_event - t->event_count + TRACE_EVENTS_BUFFER) % TRACE_EVENTS_BUFFER;

    DEBUG_PRINT("core_id=%d,n_events=%d\n", core_id, t->event_count);
    while (t->event_count > 0) {
        DEBUG_PRINT("%016llx,", t->buffer[first_event].data);
        first_event = (first_event + 1) % TRACE_EVENTS_BUFFER;
        t->event_count -= 1;
    }
    DEBUG_PRINT("\n");
    DEBUG_PRINT("\n");
}

void trace_buffer_dump() {
    DEBUG_PRINT("=================================\n");
    DEBUG_PRINT("BEGIN EVENT TRACE DUMP\n");

    for (int i = 0; i < TRACE_NUM_CORES; i++) {
        trace_buffer_dump_core(i);
    }

    DEBUG_PRINT("END EVENT TRACE DUMP\n");
    DEBUG_PRINT("=================================\n");
}
