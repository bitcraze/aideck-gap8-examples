/*
 * debug.h
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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "config.h"
#include "uart.h"

#include <pmsis.h>

#include <stdbool.h>

#ifdef VERBOSE
    #define VERBOSE_PRINT(...) printf(__VA_ARGS__)
    #define DEBUG_PRINT(...) uart_printf(__VA_ARGS__)
#else
    #define VERBOSE_PRINT(...)
    #define DEBUG_PRINT(...)
#endif

void memory_dump(pi_device_t *cluster);

// Dump debug unit registers for a given cluster core.
// This function must be called from the fabric controller and the cluster 
// must be powered up for it to work (i.e., it must be called after 
// pi_cluster_open). When the halt flag is true, this function will halt the
// core, print all debug unit registers and then resume the core. Otherwise, 
// if halt is false, this will print only the set of registers that can be 
// accessed while the core is running.
void cluster_core_dbg_dump(int core_id, bool halt);

void watchdog_reset();
void watchdog_start();

// Guarded mallocs
void *pmsis_l1_malloc_guard(size_t size);
void pmsis_l1_malloc_guard_free(void *alloc, size_t size);

void *pi_l2_malloc_guard(size_t size);
void pi_l2_malloc_guard_free(void *alloc, size_t size);

void *malloc_guard_check(void *alloc, size_t size);

// #define pi_l2_malloc(size) pi_l2_malloc_guard(size)
// #define pi_l2_free(alloc, size) pi_l2_malloc_guard_free(alloc, size)

#endif // __DEBUG_H__
