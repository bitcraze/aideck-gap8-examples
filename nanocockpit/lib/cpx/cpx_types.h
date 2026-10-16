/*
 * cpx_types.h
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

#ifndef __CPX_TYPES_H__
#define __CPX_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

#define CPX_VERSION     (0x0)

// Identify sources and destinations for CPX routing information
typedef enum {
    CPX_T_STM32     = 0x01, // The STM in the Crazyflie
    CPX_T_ESP32     = 0x02, // The ESP on the AI-deck
    CPX_T_WIFI_HOST = 0x03, // A remote computer connected via Wi-Fi
    CPX_T_GAP       = 0x04  // The GAP on the AI-deck
} cpx_target_e;

typedef enum {
    CPX_F_SYSTEM     = 0x01,
    CPX_F_CONSOLE    = 0x02,
    CPX_F_CRTP       = 0x03,
    CPX_F_WIFI_CTRL  = 0x04,
    CPX_F_APP        = 0x05,

    CPX_F_STREAMER   = 0x06,

    CPX_F_TEST       = 0x0E,
    CPX_F_BOOTLOADER = 0x0F,

    CPX_F_LAST
} cpx_function_e;

typedef struct cpx_header_s {
    cpx_target_e destination : 3;
    cpx_target_e source : 3;
    bool last_packet : 1;
    bool reserved : 1;
    cpx_function_e function : 6;
    uint8_t version : 2;
} __attribute__((packed)) cpx_header_t;

#define CPX_HEADER_INIT(/* cpx_target_e */ dest, /* cpx_function_e */ func)  \
    ((cpx_header_t){                                                         \
        .destination = dest, .source = CPX_T_GAP,                            \
        .last_packet = true, .reserved = false,                              \
        .function = func,                                                    \
        .version = CPX_VERSION                                               \
    })

#endif // __CPX_TYPES_H__
