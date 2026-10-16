/*
 * net_utils.h
 * Elia Cereda <elia.cereda@idsia.ch>
 * Alessio Burrello <alessio.burrello@unibo.it>
 *
 * Copyright (C) 2022-2025 IDSIA, USI-SUPSI
 *               2019-2020 University of Bologna
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
 */

#ifndef __NET_UTILS_H__
#define __NET_UTILS_H__

#include <stddef.h>
#include <stdint.h>

#define ASSERTION_FAILURE(...)                               \
    ({                                                       \
        printf("[ASSERT %s:%d] ", __FUNCTION__, __LINE__);   \
        printf(__VA_ARGS__);                                 \
        pmsis_exit(-1);                                      \
    })

typedef struct {
  unsigned int L3_input;
  unsigned int L3_output;
  unsigned int L3_after_weights;
  unsigned int L2_input;
  unsigned int bypass;
  unsigned int L2_output;
  unsigned int L2_weights;
  unsigned int L1_buffer;
  unsigned int ram;
  unsigned int out_mult;
  unsigned int out_shift;
  unsigned int layer_id;
} layer_args_t;

void print_perf(const char *name, const int cycles, const int macs);
void checksum(const char *name, const uint8_t *d, size_t size, uint32_t sum_true);
void checksumL3(const char *name, uint32_t L3_d, const uint8_t *L2_d, size_t size, uint32_t sum_true);

#endif // __NET_UTILS_H__
