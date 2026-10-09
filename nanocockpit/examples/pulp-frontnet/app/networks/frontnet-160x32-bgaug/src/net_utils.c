/*
 * net_utils.c
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

#include "net_utils.h"

#include "mem.h"

#include <stdio.h>

#define MIN(x, y)           \
  ({                        \
    __typeof__(x) _x = (x); \
    __typeof__(y) _y = (y); \
    _x < _y ? _x : _y;      \
  })

void print_perf(const char *name, const int cycles, const int macs) {
  // Using fixed point arithmetic because float printf is broken on the cluster
  // float perf = (float) macs / cycles;
  uint64_t perf = macs * 1000ull / cycles;
  uint32_t perf_i = (uint32_t)(perf / 1000ull);
  uint32_t perf_f = (uint32_t)(perf - perf_i * 1000ull);

  printf("\n%s performance:\n", name);
  printf("  - num cycles: %d\n", cycles);
  printf("  - MACs: %d\n", macs);
  printf("  - MAC/cycle: %u.%03u\n", perf_i, perf_f);
  printf("  - n. of Cores: %d\n", NUM_CORES);
  printf("\n");
}

void checksum(const char *name, const uint8_t *d, size_t size, uint32_t sum_true) {
  uint32_t sum = 0;
  for (int i = 0; i < size; i++) sum += d[i];

  printf("Checking %s: Checksum ", name);
  if (sum_true == sum)
    printf("OK\n");
  else
    printf("Failed: true [%u] vs. calculated [%u]\n", sum_true, sum);
}

void checksumL3(const char *name, uint32_t L3_d, const uint8_t *L2_d, size_t size, uint32_t sum_true) {
  uint32_t sum = 0;

  size_t remaining_size = size;
  while (remaining_size > 0) {
    size_t batch_size = MIN(remaining_size, 128);
    cl_ram_read(L2_d, L3_d, batch_size);
    for (int i = 0; i < batch_size; i++) sum += L2_d[i];
    L3_d += batch_size;
    remaining_size -= batch_size;
  }

  printf("Checking %s: Checksum ", name);
  if (sum_true == sum)
    printf("OK\n");
  else
    printf("Failed: true [%u] vs. calculated [%u]\n", sum_true, sum);
}
