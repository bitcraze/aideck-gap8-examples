/*
 * rng.c
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

#include "rng.h"

#include "coroutine.h"

#include <pmsis.h>

static uint32_t entropy;
static uint32_t entropy_mask;

void rng_init() {
    entropy      = 0x00000000;
    entropy_mask = 0x00000000;
}

void rng_push_entropy(uint32_t new_entropy) {
    entropy      = new_entropy;
    entropy_mask = 0xFFFFFFFF;
}

uint32_t rng_random_bits(uint8_t n_bits) {
    uint32_t mask = (1 << n_bits) - 1;
    uint32_t output = entropy & mask;
    uint32_t valid = entropy_mask & mask;

    if (valid != mask) {
        CO_ASSERTION_FAILURE("Insufficient entropy\n");
    }

    entropy      = entropy >> n_bits;
    entropy_mask = entropy_mask >> n_bits;

    return output;
}
