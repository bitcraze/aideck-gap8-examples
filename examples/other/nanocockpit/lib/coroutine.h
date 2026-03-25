/*
 * coroutine.h
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
 * STACKLESS COROUTINES OVER PMSIS TASKS
 *
 * A coroutine is a function whose execution can be suspended, while some other
 * work is performed, and later resumed. Equivalents have been added to many 
 * programming languages in the form of async/await functions.
 *
 * Inspiration:
 *   - https://en.cppreference.com/w/cpp/language/coroutines
 *   - https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 *
 * Known limitations:
 *   - Local variables in stackless coroutines are not preserved between resumes
 *     (the compiler should give a -Wmaybe-uninitialized error if you try!)
 *   - The current switch-based implementation prevents using switch statements
 *     inside an async function (can be addressed by moving to a label pointer-
 *     based implementation).
 */

#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include "config.h"
#include "list.h"
#include "utils.h"

#include <pmsis.h>

#include <stdbool.h>

#define CO_PRINT(...) printf(__VA_ARGS__)
#ifdef CO_VERBOSE
    #define CO_VERBOSE_PRINT(...) CO_PRINT(__VA_ARGS__)
#else
    #define CO_VERBOSE_PRINT(...) 
#endif
#define CO_ASSERTION_FAILURE(...)                               \
    ({                                                          \
        CO_PRINT("[ASSERT %s:%d] ", __FUNCTION__, __LINE__);    \
        CO_PRINT(__VA_ARGS__);                                  \
        pmsis_exit(-1);                                         \
    })

typedef struct co_fn_ctx_s co_fn_ctx_t;

// Coroutine function pointer
typedef void (*co_fn_t)(co_fn_ctx_t *ctx);

// Coroutine resume point
typedef int16_t co_fn_resume_t;
typedef enum {
    CO_RESUME_START = 0,
    CO_RESUME_RUNNING = -1,
    CO_RESUME_DONE = -2
} co_fn_resume_e;

// Coroutine context struct
// Contains all the data needed by an instance of a coroutine
typedef struct co_fn_ctx_s {
    co_fn_t fn;
    void *arg;

    pi_task_t resume_task;
    co_fn_resume_t resume_point;

    pi_task_t *done_task;

    // Linked list of contexts waiting on the same co_event_t
    list_el_t waiting;
} co_fn_ctx_t;

// Represents an event that can be waited for using CO_WAIT in a coroutine, 
// basically a simple extension of pi_task_t that can be waited for. 
// NOTE: the co_event_t name comes from the new name of pi_task_t in the GAP9 SDK,
//       i.e. pi_evt_t, which better captures its purpose.
typedef struct co_event_s {
    pi_task_t done_task;

    // Linked list of contexts waiting on this event
    list_head_t waiting;
} co_event_t;

static inline co_fn_ctx_t *co_fn_suspend(co_fn_ctx_t *ctx, co_fn_resume_t resume_point) {
    ctx->resume_point = resume_point;
    return ctx;
}

static inline void co_fn_resume(co_fn_ctx_t *ctx) {
    (*ctx->fn)(ctx);
}

static inline void co_fn_push_resume(co_fn_ctx_t *ctx) {
    pi_task_push(pi_task_callback(&ctx->resume_task, (pi_callback_func_t)ctx->fn, ctx));
}

// Start a new instance of a coroutine function
// 
// Params:
//   - ctx: context object used to store the execution state of the new coroutine instance. 
//          Must be kept alive until the coroutine terminates.
//   - function: pointer to the coroutine function
//   - arg: optional argument to pass to the coroutine (see the corresponding CO_FN_BEGIN for the expected type)
//   - done_task: optional pi_task_t to be triggered when the coroutine terminates
static inline void co_fn_push_start(co_fn_ctx_t *ctx, co_fn_t function, void *arg, pi_task_t *done_task) {
    if (ctx->resume_point != CO_RESUME_START && ctx->resume_point != CO_RESUME_DONE) {
        // FIXME: this assumes that ctx is zero-initialized
        CO_ASSERTION_FAILURE("Function not correctly initialized or started while already running");
    }
    
    ctx->fn = function;
    ctx->arg = arg;
    ctx->resume_point = CO_RESUME_START;
    ctx->done_task = done_task;
    list_el_init(&ctx->waiting);

    co_fn_push_resume(ctx);
}

static void co_event_callback(void *arg) {
    co_event_t *event = arg;
    list_el_t *el;

    // TODO: when a single context was waiting on this event, call directly co_fn_resume

    // Resume all contexts that were waiting on this event
    while ((el = list_pop_front(&event->waiting))) {
        co_fn_ctx_t *ctx = list_entry(el, co_fn_ctx_t, waiting);
        CO_VERBOSE_PRINT("_co_event_callback, event: %p, resuming ctx: %p\n", event, ctx);
        co_fn_push_resume(ctx);
    }
}

// Initialize a co_event_t, can be used everywhere you would use pi_task_callback
static inline pi_task_t *co_event_init(co_event_t *event) {
    list_head_init(&event->waiting);

    return pi_task_callback(&event->done_task, co_event_callback, event);
}

// Mark event as completed and resume all contexts that were waiting on it
static inline void co_event_push(co_event_t *event) {
    pi_task_push(&event->done_task);
}

// Check whether an event has already completed
static inline bool co_event_is_done(co_event_t *event) {
    return pi_task_is_done(&event->done_task);
}

static inline void co_event_wait(co_event_t *event, co_fn_ctx_t *ctx) {
    bool event_done;
    
    // Ensure that exactly one between co_event_wait and co_event_callback will call resume
    int irq = disable_irq();
    list_append(&event->waiting, &ctx->waiting);
    event_done = co_event_is_done(event);
    restore_irq(irq);

    CO_VERBOSE_PRINT("co_event_wait, ctx: %p, event: %p, event_done: %d\n", ctx, event, event_done);

    if (event_done) {
        // This event is already completed, immediately schedule a resume.
        // Still go through pi_task_push to ensure that other coroutines have
        // a chance to run even if continuing immediately.
        list_clear(&event->waiting);
        co_fn_push_resume(ctx);
    } else {
        // This event is not done yet, co_event_callback will schedule the resume.
    }
}

// Forward declaration of a coroutine function
#define CO_FN_DECLARE(fn_name)                                                      \
    static void fn_name(co_fn_ctx_t *__co_ctx)

// Begin the definition of a coroutine function
//
// Params:
//  - fn_name: name of the coroutine function
//  - arg_type: type of the argument expected by the coroutine. 
//              It must be a pointer or pointer-sized.
//  - arg_name: name of the argument variable to be used inside the coroutine body
#define CO_FN_BEGIN(fn_name, arg_type, arg_name)                                    \
    CO_FN_DECLARE(fn_name) {                                                        \
        /* Ensure -Wmaybe-uninitialized is always an error to catch local */        \
        /* variables used across suspension points. */                              \
        _Pragma("GCC diagnostic push")                                              \
        _Pragma("GCC diagnostic error \"-Wuninitialized\"")                         \
        _Pragma("GCC diagnostic error \"-Wmaybe-uninitialized\"")                   \
                                                                                    \
        co_fn_resume_t __co_resume = __co_ctx->resume_point;                        \
        __co_ctx->resume_point = CO_RESUME_RUNNING;                                 \
                                                                                    \
        CO_VERBOSE_PRINT(                                                           \
            "%s, ctx: %p, resume_point: %d\n",                                      \
            __FUNCTION__, __co_ctx, __co_resume                                     \
        );                                                                          \
                                                                                    \
        arg_type arg_name = (arg_type)__co_ctx->arg;                                \
                                                                                    \
        switch (__co_resume) {                                                      \
        case CO_RESUME_START:                                                       \
                                                                                    \
        /* Ensure users put a brace after CO_FN_BEGIN and close the function */     \
        /* with a matching call to CO_FN_END */                                     \
        do

// Define a suspension point in the current coroutine, used by the implementation 
// of CO_YIELD, CO_WAIT, etc.
#define CO_RESUME_POINT_BEGIN()                                                     \
        ((co_fn_resume_t)__LINE__)

#define CO_RESUME_POINT_END()                                                       \
        return;                                                                     \
                                                                                    \
        case __LINE__:

// Suspend the execution of the current coroutine and give control back to the 
// scheduler, giving a chance for other tasks to execute.
// The coroutine is immediately resumed as soon as the current tasks are processed
// Rule of thumb: aim to call this at least once every few hundred micro-seconds to
// ensure everything stays responsive
#define CO_YIELD()                                                                  \
        __co_resume = CO_RESUME_POINT_BEGIN();                                      \
        co_fn_push_resume(co_fn_suspend(__co_ctx, __co_resume));                    \
        CO_RESUME_POINT_END();

// Suspend the execution of the current coroutine until the given co_event_t is done
#define CO_WAIT(/* co_event_t * */ event)                                           \
        __co_resume = CO_RESUME_POINT_BEGIN();                                      \
        co_event_wait(event, co_fn_suspend(__co_ctx, __co_resume));                 \
        CO_RESUME_POINT_END();

// Terminate the execution of the current coroutine and, if needed, notify the caller
#define CO_RETURN()                                                                 \
        __co_ctx->resume_point = CO_RESUME_DONE;                                    \
                                                                                    \
        if (__co_ctx->done_task) {                                                  \
            pi_task_push(__co_ctx->done_task);                                      \
        }                                                                           \
                                                                                    \
        return;                                                                     \

// End the definition of a coroutine function
#define CO_FN_END()                                                                 \
        /* Ensure that users put a brace before CO_FN_END and that the function */  \
        /* was opened with a matching call to CO_FN_BEGIN */                        \
        while (false);                                                              \
                                                                                    \
        /* Mark the coroutine as concluded and notify the caller */                 \
        CO_RETURN();                                                                \
                                                                                    \
        case CO_RESUME_RUNNING:                                                     \
            CO_ASSERTION_FAILURE(                                                   \
                "Function resumed without being properly suspended first.\n"        \
            );                                                                      \
                                                                                    \
        case CO_RESUME_DONE:                                                        \
            CO_ASSERTION_FAILURE(                                                   \
                "Function resumed after having concluded.\n"                        \
            );                                                                      \
                                                                                    \
        default:                                                                    \
            CO_ASSERTION_FAILURE(                                                   \
                "Function resumed from invalid resume point %d.\n",                 \
                __co_resume                                                         \
            );                                                                      \
        }                                                                           \
                                                                                    \
        _Pragma("GCC diagnostic pop")                                               \
    }

#endif /* __COROUTINE_H__ */
