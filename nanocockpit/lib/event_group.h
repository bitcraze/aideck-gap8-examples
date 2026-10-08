/*
 * event_group.h
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
 * EVENT GROUP
 *
 * An event group is a collection of bits to which an application can assign a 
 * meaning. Tasks can then wait for conditions to be met
 *
 * Inspiration:
 *   - https://www.freertos.org/FreeRTOS-Event-Groups.html
 *
 * Known limitations:
 *   - Wait timeouts are not currently supported
 *   - Only a single active waiter on a given event group is currently supported
 *   - CO_WAIT_MODE_CLEAR is not implemented
 */

#ifndef __EVENT_GROUP_H__
#define __EVENT_GROUP_H__

#include "coroutine.h"
#include "list.h"

typedef uint32_t co_event_mask_t;
static const co_event_mask_t CO_EVENT_MASK_NONE = 0;

typedef enum co_wait_mode_e {
    // Wait for all/any bits in wait_mask to be set
    CO_WAIT_MODE_ANY      = 0 << 0,
    CO_WAIT_MODE_ALL      = 1 << 0,

    // Clear bits in wait_mask when returning after waiting (TODO: not implemented)
    CO_WAIT_MODE_NO_CLEAR = 0 << 1,
    // CO_WAIT_MODE_CLEAR = 1 << 1,
} co_wait_mode_e;

typedef struct co_event_group_s {
    co_event_mask_t mask;
    
    co_fn_ctx_t *wait_ctx;
    co_event_mask_t wait_mask;
    co_wait_mode_e wait_mode;
} co_event_group_t;

static inline void co_event_group_init(co_event_group_t *event_group) {
    event_group->mask = CO_EVENT_MASK_NONE;
    event_group->wait_ctx = NULL;
    event_group->wait_mask = CO_EVENT_MASK_NONE;
    event_group->wait_mode = CO_WAIT_MODE_ANY;
}

static inline bool co_event_group_test(co_event_group_t *event_group, co_event_mask_t wait_mask, co_wait_mode_e wait_mode) {
    if (wait_mode == CO_WAIT_MODE_ANY) {
        return (event_group->mask & wait_mask) != 0;
    } else if (wait_mode == CO_WAIT_MODE_ALL) {
        return (event_group->mask & wait_mask) == wait_mask;
    } else {
        CO_ASSERTION_FAILURE("Unknown wait mode %d.\n", wait_mode);
    }
}

static inline void co_event_group_update(co_event_group_t *event_group) {
    if (event_group->wait_ctx == NULL) {
        return;
    }

    bool wait_done = co_event_group_test(event_group, event_group->wait_mask, event_group->wait_mode);
    if (wait_done) {
        co_fn_push_resume(event_group->wait_ctx);
        event_group->wait_ctx = NULL;
    }
}

static inline void co_event_group_set(co_event_group_t *event_group, co_event_mask_t set_mask) {
    event_group->mask |= set_mask;
    co_event_group_update(event_group);
}

static inline co_event_mask_t co_event_group_get(co_event_group_t *event_group, co_event_mask_t get_mask) {
    co_event_mask_t mask = event_group->mask & get_mask;
    CO_VERBOSE_PRINT("co_event_group_get, event group: %p, get mask: %d, current mask: %d\n", event_group, get_mask, mask);
    return mask;
}

static inline co_event_mask_t co_event_group_clear(co_event_group_t *event_group, co_event_mask_t clear_mask) {
    co_event_mask_t current_mask = event_group->mask & clear_mask;
    event_group->mask &= ~current_mask;
    return current_mask;
}

static inline void co_event_group_wait(co_event_group_t *event_group, co_fn_ctx_t *ctx, co_event_mask_t wait_mask, co_wait_mode_e wait_mode) {
    bool wait_done;
    
    // Ensure that exactly one between co_event_group_wait and co_event_group_update will call resume
    int irq = disable_irq();
    {
        if (event_group->wait_ctx) {
            // TODO: I don't need multiple waiters at the moment
            CO_ASSERTION_FAILURE("Multiple waits on a co_event_group_t, not implemented yet.\n");
        }

        event_group->wait_ctx = ctx;
        event_group->wait_mask = wait_mask;
        event_group->wait_mode = wait_mode;

        wait_done = co_event_group_test(event_group, wait_mask, wait_mode);
    }
    restore_irq(irq);

    CO_VERBOSE_PRINT("co_event_group_wait, ctx: %p, event group: %p, wait mask: %d, wait mode: %d, event_done: %d\n", ctx, event_group, wait_mask, wait_mode, wait_done);

    if (wait_done) {
        // Wait is already completed, immediately schedule a resume.
        // Still go through pi_task_push to ensure that other coroutines have
        // a chance to run even if continuing immediately.
        event_group->wait_ctx = NULL;
        co_fn_push_resume(ctx);
    } else {
        // Wait is not done yet, co_event_group_update will schedule the resume.
    }
}

// INTEROPERATION WITH COROUTINE.H

// Suspend the current coroutine until the desired condition is met
//
// Params:
//   - event_group: event group object to wait on
//   - wait_mask: pointer to bitmask of events to consider. After resuming contains the current state of the considered events
//   - wait_mode: whether any or all bits must be set before resuming
#define CO_WAIT_GROUP(/* co_event_group_t * */ event_group, /* co_event_mask_t * */ wait_mask, /* co_wait_mode_e */ wait_mode)  \
        __co_resume = CO_RESUME_POINT_BEGIN();                                                                                  \
        co_event_group_wait(event_group, co_fn_suspend(__co_ctx, __co_resume), *wait_mask, wait_mode);                          \
        CO_RESUME_POINT_END();                                                                                                  \
        *wait_mask = co_event_group_get(event_group, *wait_mask);

// Convenience macro that hard-codes CO_WAIT_MODE_ANY
#define CO_WAIT_GROUP_ANY(/* co_event_group_t * */ event_group, /* co_event_mask_t * */ wait_mask)                              \
    CO_WAIT_GROUP(event_group, wait_mask, CO_WAIT_MODE_ANY)

// Convenience macro that hard-codes CO_WAIT_MODE_ALL
#define CO_WAIT_GROUP_ALL(/* co_event_group_t * */ event_group, /* co_event_mask_t * */ wait_mask)                              \
    CO_WAIT_GROUP(event_group, wait_mask, CO_WAIT_MODE_ALL)

#endif /* __EVENT_GROUP_H__ */
