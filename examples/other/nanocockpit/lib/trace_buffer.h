/*
 * trace_buffer.h
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

/*
 * CIRCULAR BUFFER TRACING
 *
 * Low-overhead event tracing over a circular buffer that can be dumped
 * 
 */

#ifndef __TRACE_BUFFER_H__
#define __TRACE_BUFFER_H__

#include "config.h"
#include "utils.h"

#include <pmsis.h>

#include <stdbool.h>

#define TRACE_NUM_CORES             (10)
#define TRACE_EVENTS_BUFFER         (768)
#define TRACE_EVENTS_PERF_COUNTER   (PI_PERF_CYCLES)

typedef enum trace_evt_e {
    TRACE_EVT_SYNC = 0,
} __attribute__((packed)) trace_evt_e;

typedef enum trace_state_e {
    TRACE_MARKER,
    TRACE_BEGIN,
    TRACE_END,
} __attribute__((packed)) trace_state_e;

typedef struct trace_evt_s {
    union {
        uint64_t data;
        struct {
            trace_evt_e event;
            trace_state_e state;
            uint16_t context;
            uint32_t perf_counter;
        };
    };
} trace_evt_t;

typedef struct trace_buffer_s {
    bool started;
    int next_event;
    int event_count;
    trace_evt_t buffer[TRACE_EVENTS_BUFFER];
} trace_buffer_t;

void trace_buffer_init(int core_id, pi_device_t *cluster);
void trace_buffer_start();
void trace_buffer_dump();

// Record an event
static inline void trace_event(trace_evt_e event, trace_state_e state, uint16_t context);

// Record a sync event with a global timestamp
static inline void trace_sync();

// Implementation

extern trace_buffer_t *trace_buffers[TRACE_NUM_CORES];

static inline void trace_push_event(trace_evt_t event) {
    int core_id = pi_core_id();
    trace_buffer_t *t = trace_buffers[core_id];

    if (t == NULL) {
        ASSERTION_FAILURE("Trace buffer for core %d not initialized\n", core_id);
    } else if (!t->started) {
        ASSERTION_FAILURE("Trace buffer for core %d not started\n", core_id);
    }

    t->buffer[t->next_event] = event;
    t->next_event = (t->next_event + 1) % TRACE_EVENTS_BUFFER;
    t->event_count = MIN(t->event_count + 1, TRACE_EVENTS_BUFFER);
}

static inline void trace_event(trace_evt_e event, trace_state_e state, uint16_t context) {
    uint32_t perf_counter = pi_perf_read(TRACE_EVENTS_PERF_COUNTER);
    trace_push_event((trace_evt_t){
        .event = event, .state = state, .context = context, .perf_counter = perf_counter
    });
}

static inline void trace_sync() {
    pi_perf_stop();
    uint32_t time = pi_time_get_us();
    uint16_t time_lo = (time >>  0) & 0xffff;
    uint16_t time_hi = (time >> 16) & 0xffff;
    trace_event(TRACE_EVT_SYNC,  TRACE_BEGIN, time_lo);

    pi_perf_reset();
    pi_perf_start();
    trace_event(TRACE_EVT_SYNC, TRACE_END, time_hi);
}

#endif // __TRACE_BUFFER_H__
