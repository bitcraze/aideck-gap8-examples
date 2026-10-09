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
#include "queue.h"

#include <pmsis.h>

static queue_async_t q;

static co_fn_ctx_t producer_ctx;
static co_fn_ctx_t consumer_ctx;

static co_event_t producer_done;
static co_event_t consumer_done;

static co_event_t producer_step;
static co_event_t consumer_step;

static int *a;

static int *b;
static int *c;
static int *d;

static int *e;

static const int *a1;

static const int *b1;

static const int *c1;
static const int *d1;
static const int *e1;

CO_FN_BEGIN(producer_task, void *, arg)
{
    static co_event_t queue_done;

    printf("[producer] initial queue count %d\n", queue_async_get_count(&q));

    if (queue_async_get_count(&q) != 0) {
        pmsis_exit(1);
    }

    co_event_push(&producer_step);

    CO_WAIT(&consumer_step);
    co_event_init(&consumer_step);

    queue_async_push_acquire(&q, (void **)&a, co_event_init(&queue_done));
    CO_WAIT(&queue_done);
    *a = 1;
    queue_async_push_commit(&q, a);

    printf("[producer] pushed element, queue count %d\n", queue_async_get_count(&q));

    CO_WAIT(&consumer_step);
    co_event_init(&consumer_step);

    queue_async_push_acquire(&q, (void **)&b, co_event_init(&queue_done));
    CO_WAIT(&queue_done);
    *b = 2;
    queue_async_push_commit(&q, b);
    
    queue_async_push_acquire(&q, (void **)&c, co_event_init(&queue_done));
    CO_WAIT(&queue_done);
    *c = 3;
    queue_async_push_commit(&q, c);

    queue_async_push_acquire(&q, (void **)&d, co_event_init(&queue_done));
    CO_WAIT(&queue_done);
    *d = 4;
    queue_async_push_commit(&q, d);

    printf("[producer] filled queue, queue count %d\n", queue_async_get_count(&q));

    queue_async_push_acquire(&q, (void **)&e, co_event_init(&queue_done));

    if (co_event_is_done(&queue_done)) {
        //  Queue is full now, need to wait for the consumer
        pmsis_exit(3);
    }
    printf("[producer] waiting consumer\n");

    co_event_push(&producer_step);

    CO_WAIT(&queue_done);

    if (e != b) {
        pmsis_exit(6);
    }

    *e = 5;
    queue_async_push_commit(&q, e);

    printf("[producer] pushed element e, queue count %d\n", queue_async_get_count(&q));

    co_event_push(&producer_step);
}
CO_FN_END()

CO_FN_BEGIN(consumer_task, void *, arg)
{
    static co_event_t queue_done;

    CO_WAIT(&producer_step);
    co_event_init(&producer_step);

    queue_async_pop_consume(&q, (const void **)&a1, co_event_init(&queue_done));
    printf("[consumer] waiting to pop element, queue count %d\n", queue_async_get_count(&q));

    co_event_push(&consumer_step);
    CO_WAIT(&queue_done);

    printf("[consumer] popped element %08x, value %d\n", a1, *a1);
    
    if (a1 != a || *a1 != 1) {
        pmsis_exit(2);
    }

    queue_async_pop_release(&q, a1);
    printf("[consumer] released element, queue count %d\n", queue_async_get_count(&q));

    co_event_push(&consumer_step);

    CO_WAIT(&producer_step);
    co_event_init(&producer_step);

    printf("[consumer] waited producer, queue count %d\n", queue_async_get_count(&q));

    queue_async_pop_consume(&q, (const void **)&b1, co_event_init(&queue_done));
    CO_WAIT(&queue_done);

    if (b1 != b || *b1 != 2) {
        pmsis_exit(5);
    }

    queue_async_pop_release(&q, b1);

    CO_WAIT(&producer_step);
    co_event_init(&producer_step);

    queue_async_pop_consume(&q, (const void **)&c1, co_event_init(&queue_done));
    CO_WAIT(&queue_done);

    queue_async_pop_consume(&q, (const void **)&d1, co_event_init(&queue_done));
    CO_WAIT(&queue_done);

    queue_async_pop_consume(&q, (const void **)&e1, co_event_init(&queue_done));
    CO_WAIT(&queue_done);

    queue_async_pop_release(&q, c1);
    queue_async_pop_release(&q, d1);
    queue_async_pop_release(&q, e1);
}
CO_FN_END()

void main_task() {
    queue_async_init(&q, 3, sizeof(int));

    co_event_init(&producer_step);
    co_event_init(&consumer_step);

    co_fn_push_start(&producer_ctx, producer_task, NULL, co_event_init(&producer_done));
    co_fn_push_start(&consumer_ctx, consumer_task, NULL, co_event_init(&consumer_done));

    while (!co_event_is_done(&producer_done) || !co_event_is_done(&consumer_done)) {
        pi_yield();
    }

    pmsis_exit(0);
}

int main(void) {
    printf("\n\n\t *** PMSIS Kickoff ***\n\n");
    return pmsis_kickoff((void *)main_task);
}
