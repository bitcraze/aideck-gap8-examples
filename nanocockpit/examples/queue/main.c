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

void main_task() {
    queue_t q;
    queue_init(&q, 3, sizeof(int));

    printf("initial queue count %d\n", queue_get_count(&q));

    int *a = queue_push_acquire(&q, false);
    *a = 1;

    if (queue_get_count(&q) != 0 || queue_peek(&q) != NULL) {
        pmsis_exit(1);
    }

    queue_push_commit(&q, a);

    const int *a1 = queue_peek(&q);
    if (a1 != a) {
        pmsis_exit(2);
    }

    printf("queue count %d, a1 %p, *a1 %d\n", queue_get_count(&q), a1, *a1);

    const int *a2 = queue_pop_consume(&q);
    if (a2 != a || *a2 != 1) {
        pmsis_exit(3);
    }

    if (queue_get_count(&q) != 0 || queue_peek(&q) != NULL) {
        pmsis_exit(4);
    }

    printf("queue count %d, a2 %p *a2 %d\n", queue_get_count(&q), a2, *a2);

    queue_pop_release(&q, a2);

    int *b = queue_push_acquire(&q, false);
    int *c = queue_push_acquire(&q, false);
    int *d = queue_push_acquire(&q, false);
    *b = 2;
    *c = 3;
    *d = 4;

    printf("queue count full acquired: %d, b %p, c %p, d %p\n", queue_get_count(&q), b, c, d);

    if (b != a + 1 || c != b + 1 || d != a) {
        pmsis_exit(5);
    }

    int *e = queue_push_acquire(&q, false);

    if (e != NULL) {
        pmsis_exit(6);
    }

    e = queue_push_acquire(&q, true);

    if (e != NULL) {
        pmsis_exit(7);
    }

    queue_push_commit(&q, b);
    queue_push_commit(&q, c);
    queue_push_commit(&q, d);

    printf("queue count full: %d, b %p, c %p, d %p\n", queue_get_count(&q), b, c, d);

    if (queue_get_count(&q) != 3) {
        pmsis_exit(8);
    }

    e = queue_push_acquire(&q, true);
    *e = 5;

    if (e != b) {
        pmsis_exit(11);
    }

    if (queue_get_count(&q) != 2) {
        pmsis_exit(9);
    }

    queue_push_discard(&q, e);

    if (queue_get_count(&q) != 2) {
        pmsis_exit(10);
    }

    int *e_ = queue_push_acquire(&q, true);

    if (queue_get_count(&q) != 2 || e_ != e) {
        pmsis_exit(11);
    }

    queue_push_commit(&q, e);

    printf("queue count after overwrite: %d\n", queue_get_count(&q));

    const int *c1 = queue_pop_consume(&q);

    if (c1 != c || *c1 != 3) {
        pmsis_exit(12);
    }

    const int *d1 = queue_pop_consume(&q);
    const int *e1 = queue_pop_consume(&q);

    printf("final queue count: %d, c1 %p, d1 %p, e1 %p\n", queue_get_count(&q), c1, d1, e1);

    if (d1 != d || *d1 != 4 || e1 != e || *e1 != 5) {
        pmsis_exit(13);
    }

    int *f = queue_push_acquire(&q, true);

    if (f != NULL) {
        pmsis_exit(14);
    }

    queue_pop_release(&q, c1);

    f = queue_push_acquire(&q, true);

    if (f != c1) {
        pmsis_exit(15);
    }

    queue_pop_release(&q, d1);
    queue_pop_release(&q, e1);

    pmsis_exit(0);
}

int main(void) {
    printf("\n\n\t *** PMSIS Kickoff ***\n\n");
    return pmsis_kickoff((void *)main_task);
}
