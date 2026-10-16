/*
 * uart_protocol.h
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

#ifndef __UART_PROTOCOL_H__
#define __UART_PROTOCOL_H__

#include "config.h"
#include "coroutine.h"
#include "utils.h"

#include <pmsis.h>

#include <stdint.h>

#define UART_BUFFER_LENGTH 128
#define UART_HEADER_LENGTH 4
#define UART_CHECKSUM_LENGTH sizeof(uint32_t)
#define UART_READ_SIZE 4

#define UART_STATE_MSG_HEADER "!STA"
typedef struct state_msg_s {
  // STM32 timestamp [ticks]
  uint32_t timestamp;

  // position [mm]
  int16_t x;
  int16_t y;
  int16_t z;

  // velocity [mm/s]
  int16_t vx;
  int16_t vy;
  int16_t vz;

  // acceleration [mm/s^2]
  int16_t ax;
  int16_t ay;
  int16_t az;

  // compressed quaternion, see quatcompress.h (elements stored as xyzw)
  int32_t quat;

  // angular velocity [mrad/s]
  int16_t rateRoll;
  int16_t ratePitch;
  int16_t rateYaw;
} __attribute__((packed)) state_msg_t;

#define UART_RNG_MSG_HEADER "!RNG"
typedef struct rng_msg_s {
  uint32_t entropy;
} __attribute__((packed)) rng_msg_t;

#define UART_TOF_MSG_HEADER "!TOF"
typedef struct tof_msg_s {
  uint8_t resolution;
  uint8_t _padding[3];

  uint8_t data[64];
} __attribute__((packed)) tof_msg_t;

#define UART_INFERENCE_OUTPUT_MSG_HEADER "\x90\x19\x8\x31"
typedef struct {
  float x;
  float y;
  float z;
  float phi;
} __attribute__((packed)) inference_output_msg_t;

#define UART_INFERENCE_STAMPED_MSG_HEADER "\x90\x19\x8\x32"
typedef struct {
  uint32_t stm32_timestamp;
  float x;
  float y;
  float z;
  float phi;
} __attribute__((packed)) inference_stamped_msg_t;

typedef struct uart_msg_s {
    uint8_t header[UART_HEADER_LENGTH];
    union {
        state_msg_t state;
        rng_msg_t rng;
        tof_msg_t tof;
        inference_stamped_msg_t inference_stamped;
    };
    uint32_t checksum;
    uint32_t recv_timestamp;
} __attribute__((packed)) uart_msg_t;

typedef struct uart_s uart_t;

typedef struct uart_protocol_s {
    uart_t *uart;
    co_fn_ctx_t protocol_ctx;

    uint8_t buffer[UART_BUFFER_LENGTH];

    co_event_t done_event;

    co_fn_t message_callback;
    co_fn_ctx_t message_ctx;

    uart_msg_t tx_message;
} uart_protocol_t;

void uart_protocol_init(uart_protocol_t *protocol, uart_t *uart, co_fn_t callback);
void uart_protocol_start(uart_protocol_t *protocol);

void uart_protocol_send_inference_async(uart_protocol_t *protocol, inference_stamped_msg_t *msg, pi_task_t *done_task);

#endif // __UART_PROTOCOL_H__
