/*
 * list.h
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
 * INTRUSIVE LINKED LIST
 *
 * See also:
 *    - https://www.data-structures-in-practice.com/intrusive-linked-lists/
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

typedef struct list_el_s list_el_t;

typedef struct list_el_s {
    list_el_t *next;
} list_el_t;

typedef struct list_head_s {
    list_el_t *first;
} list_head_t;

static inline void list_el_init(list_el_t *el) {
    el->next = NULL;
}

static inline void list_head_init(list_head_t *head) {
    head->first = NULL;
}

static inline void list_append(list_head_t *head, list_el_t *el) {
    if (!head->first) {
        head->first = el;
    } else {
        list_el_t *last = head->first;

        while (last->next) {
            last = last->next;
        }

        last->next = el;
    }
}

static inline list_el_t *list_pop_front(list_head_t *head) {
    list_el_t *el = head->first;

    if (el) {
        head->first = el->next;
        el->next = NULL;
    }

    return el;
}

static inline void list_clear(list_head_t *head) {
    while (head->first) {
        list_pop_front(head);
    }

    head->first = NULL;
}

#define list_entry(el, type, member) \
    ((type *)((void *)el - offsetof(type, member)))

#endif /* __LIST_H__ */
