/*
 * queue.c
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

// See: https://en.wikipedia.org/wiki/Circular_buffer

#include "queue.h"

#include "config.h"
#include "debug.h"

#include <pmsis.h>

#define QUEUE_PRINT(...) printf(__VA_ARGS__)
#ifdef QUEUE_VERBOSE
    #define QUEUE_VERBOSE_PRINT(...) QUEUE_PRINT(__VA_ARGS__)
#else
    #define QUEUE_VERBOSE_PRINT(...) 
#endif
#define QUEUE_ASSERTION_FAILURE(...)                            \
    ({                                                          \
        QUEUE_PRINT("[ASSERT %s:%d] ", __FUNCTION__, __LINE__); \
        QUEUE_PRINT(__VA_ARGS__);                               \
        pmsis_exit(-1);                                         \
    })

void queue_init(queue_t *q, int capacity, int el_size) {
    q->capacity = capacity;
    q->el_size = el_size;

    q->start = 0;
    q->end = 0;
    q->count = 0;

    q->end_acq = 0;
    q->count_acq = 0;

    q->start_consume = 0;
    q->count_consume = 0;
    
    // q->buffer = pi_fc_l1_malloc(capacity * el_size);
    // q->buffer = pi_l2_malloc_guard(capacity * el_size);
    q->buffer = pi_l2_malloc(capacity * el_size);

    if (!q->buffer) {
        QUEUE_ASSERTION_FAILURE("Queue buffer allocation failed\n");
    }
}

void queue_free(queue_t *q) {
    // pi_fc_l1_free(q->buffer, q->capacity * q->el_size);
    // pi_l2_malloc_guard_free(q->buffer, q->capacity * q->el_size);
    pi_l2_free(q->buffer, q->capacity * q->el_size);
}

void *_queue_get_el(queue_t *q, int i) {
    // malloc_guard_check(q->buffer, q->capacity * q->el_size);

    if (i < 0 || i >= q->capacity) {
        QUEUE_ASSERTION_FAILURE("Queue access out of bounds\n");
    }

    return q->buffer + i * q->el_size;
}

int queue_get_count(queue_t *q) {
    return q->count;
}

void *queue_push_acquire(queue_t *q, bool overwrite) {
    int next_end_acq = (q->end_acq + 1) % q->capacity;

    QUEUE_VERBOSE_PRINT("[queue_push_acquire]: start %d, end %d, end_acq %d, next_end_acq %d\n", q->start, q->end, q->end_acq, next_end_acq);

    if (q->count_acq + q->count + q->count_consume == q->capacity) {
        if (!overwrite) {
            // Caller does not want to overwrite
            return NULL;
        } else if (q->count == 0) {
            // Queue is full of in-use elements, nothing to overwrite
            return NULL;
        }

        const void *el = queue_pop_consume(q);
        queue_pop_release(q, el);
    }

    void *el = _queue_get_el(q, q->end_acq);

    q->end_acq = next_end_acq;
    q->count_acq += 1;

    return el;
}

void queue_push_commit(queue_t *q, void *el) {
    void *expected_el = _queue_get_el(q, q->end);

    QUEUE_VERBOSE_PRINT("[queue_push_commit]: start %d, end %d, end_acq %d, el %08x, expected_el %08x\n", q->start, q->end, q->end_acq, el, expected_el);

    if (q->count_acq == 0) {
        QUEUE_ASSERTION_FAILURE("No pending acquired element\n");
    }

    if (el != expected_el) {
        QUEUE_ASSERTION_FAILURE("Attempt to commit element out-of-order\n");
    }

    q->end = (q->end + 1) % q->capacity;
    q->count += 1;
    q->count_acq -= 1;
}

void queue_push_discard(queue_t *q, void *el) {
    int prev_end_acq = (q->end_acq - 1 + q->capacity) % q->capacity;
    void *expected_el = _queue_get_el(q, prev_end_acq);

    QUEUE_VERBOSE_PRINT("[queue_push_discard]: start %d, end %d, end_acq %d, el %08x, expected_el %08x\n", q->start, q->end, q->end_acq, el, expected_el);

    if (q->count_acq == 0) {
        QUEUE_ASSERTION_FAILURE("No pending acquired element\n");
    }

    if (el != expected_el) {
        QUEUE_ASSERTION_FAILURE("Attempt to discard element out-of-order\n");
    }

    q->end_acq = prev_end_acq;
    q->count_acq -= 1;
}

const void *queue_peek(queue_t *q) {
    if (q->count == 0) {
        return NULL;
    }

    void *el = _queue_get_el(q, q->start);

    return el;
}

const void *queue_pop_consume(queue_t *q) {
    int next_start = (q->start + 1) % q->capacity;

    QUEUE_VERBOSE_PRINT("[queue_pop_consume]: start %d, end %d, start_consume %d, next_start %d\n", q->start, q->end, q->start_consume, next_start);

    const void *el = queue_peek(q);

    if (el) {
        q->start = next_start;
        q->count -= 1;
        q->count_consume += 1;
    }

    return el;
}

void queue_pop_release(queue_t *q, const void *el) {
    void *expected_el = _queue_get_el(q, q->start_consume);

    QUEUE_VERBOSE_PRINT("[queue_pop_release]: start %d, end %d, start_consume %d, el %08x, expected_el %08x\n", q->start, q->end, q->start_consume, el, expected_el);

    if (q->count_consume == 0) {
        QUEUE_ASSERTION_FAILURE("No pending consumed element\n");
    }

    if (el != expected_el) {
        QUEUE_ASSERTION_FAILURE("Attempt to release element out-of-order\n");
    }

    q->start_consume = (q->start_consume + 1) % q->capacity;
    q->count_consume -= 1;
}

// QUEUE ASYNC

void queue_async_init(queue_async_t *q, int capacity, int el_size) {
    queue_init(&q->q, capacity, el_size);

    co_event_init(&q->queue_free);
    co_event_push(&q->queue_free);

    co_event_init(&q->queue_ready);

    q->push_el = NULL;
    q->pop_el = NULL;
}

int queue_async_get_count(queue_async_t *q) {
    return queue_get_count(&q->q);
}

CO_FN_BEGIN(queue_async_push_task, queue_async_t *, q)
{
    CO_WAIT(&q->queue_free);

    void **el = q->push_el;
    
    *el = queue_push_acquire(&q->q, /* overwrite */ false);

    if (*el == NULL) {
        QUEUE_ASSERTION_FAILURE("queue_free asserted but queue is full\n");
    }

    q->push_el = NULL;
}
CO_FN_END()

void queue_async_push_acquire(queue_async_t *q, void **el, pi_task_t *done_task) {
    *el = queue_push_acquire(&q->q, /* overwrite */ false);

    if (*el != NULL) {
        // Free space available in the queue, we are done
        if (done_task != NULL) {
            pi_task_push(done_task);
        }
        return;
    }

    if (done_task == NULL) {
        // Caller does not want to wait, nothing to do
        return;
    }

    if (q->push_el != NULL) {
        QUEUE_ASSERTION_FAILURE("Another producer already waiting to push\n");
    }

    // Wait for space to free up
    q->push_el = el;
    co_event_init(&q->queue_free);
    co_fn_push_start(&q->push_ctx, queue_async_push_task, q, done_task);
}

void queue_async_push_commit(queue_async_t *q, void *el) {
    queue_push_commit(&q->q, el);
    co_event_push(&q->queue_ready);
}

void queue_async_push_discard(queue_async_t *q, void *el) {
    queue_push_discard(&q->q, el);
}

CO_FN_BEGIN(queue_async_pop_task, queue_async_t *, q)
{
    CO_WAIT(&q->queue_ready);
    
    const void **el = q->pop_el;
    
    *el = queue_pop_consume(&q->q);

    if (*el == NULL) {
        QUEUE_ASSERTION_FAILURE("queue_ready asserted but queue is empty\n");
    }

    q->pop_el = NULL;
}
CO_FN_END()

void queue_async_pop_consume(queue_async_t *q, const void **el, pi_task_t *done_task) {
    *el = queue_pop_consume(&q->q);

    if (*el != NULL) {
        // Element already available in the queue, we are done
        if (done_task != NULL) {
            pi_task_push(done_task);
        }
        return;
    }

    if (done_task == NULL) {
        // Caller does not want to wait, nothing to do
        return;
    }

    if (q->pop_el != NULL) {
        QUEUE_ASSERTION_FAILURE("Another consumer already waiting to pop\n");
    }

    // Wait for space to free up
    q->pop_el = el;
    co_event_init(&q->queue_ready);
    co_fn_push_start(&q->pop_ctx, queue_async_pop_task, q, done_task);
}

void queue_async_pop_release(queue_async_t *q, const void *el) {
    queue_pop_release(&q->q, el);
    co_event_push(&q->queue_free);
}
