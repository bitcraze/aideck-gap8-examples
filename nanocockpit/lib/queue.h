/*
 * queue.h
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
 * QUEUE
 *
 * FIFO queue implementation backed by a circular buffer
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "coroutine.h"

#include <pmsis.h>

#include <stdbool.h>

typedef struct queue_s {
    int capacity;
    int el_size;

    int start, end;
    int count;

    int end_acq;
    int count_acq;

    int start_consume;
    int count_consume;

    void *buffer;
} queue_t;

void queue_init(queue_t *q, int capacity, int el_size);
int  queue_get_count(queue_t *q);

// Push a new element to the queue. If the queue is full and overwrite is set, the
// queue will discard the oldest element and push the new element in its place. 
// When overwrite is not set, this function will return NULL when the queue is full.
// To avoid needless copies, the caller should first call this function to acquire
// the memory where the element should be stored and write it in place.
void *queue_push_acquire(queue_t *q, bool overwrite);
void queue_push_commit(queue_t *q, void *el);
void queue_push_discard(queue_t *q, void *el);

// Access the first (oldest) element in the queue, without removing it
const void *queue_peek(queue_t *q);

// Return the first (oldest) element in the queue and remove it
const void *queue_pop_consume(queue_t *q);
void queue_pop_release(queue_t *q, const void *el);

/*
 * ASYNC QUEUE
 *
 *  Single-producer single-consumer asynchronous queue
 */

typedef struct queue_async_s {
    queue_t q;

    co_event_t queue_free;
    co_event_t queue_ready;

    co_fn_ctx_t push_ctx;
    void **push_el;

    co_fn_ctx_t pop_ctx;
    const void **pop_el;
} queue_async_t;

void queue_async_init(queue_async_t *q, int capacity, int el_size);
int  queue_async_get_count(queue_async_t *q);

// Push a new element to the queue.
void queue_async_push_acquire(queue_async_t *q, void **el, pi_task_t *done_task);
void queue_async_push_commit(queue_async_t *q, void *el);
void queue_async_push_discard(queue_async_t *q, void *el);

// Return the first (oldest) element in the queue and remove it
void queue_async_pop_consume(queue_async_t *q, const void **el, pi_task_t *done_task);
void queue_async_pop_release(queue_async_t *q, const void *el);

#endif // __QUEUE_H__
