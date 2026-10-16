/*
 * cpx_spi.h
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

#ifndef __CPX_SPI_H__
#define __CPX_SPI_H__

#include "cpx_types.h"
#include "coroutine.h"
#include "event_group.h"

// CPX SPI header must be >= 4 bytes long and aligned to 4 bytes to account for GAP SPI limitations
typedef struct cpx_spi_header_s {
    // Length of the cpx_spi payload (maximum supported size is CPX_SPI_MTU)
    uint16_t length;

    // CPX header with routing information
    cpx_header_t cpx;
} __attribute__((packed)) cpx_spi_header_t;

// Maximum total size of a CPX SPI packet (header + payload)
// Currently: maximum supported DMA transfer length by ESP32 (SPI_MAX_DMA_LEN = 4092 bytes)
#define CPX_SPI_MAX_PACKET_LENGTH (4092)

// Maximum payload size of a CPX SPI packet
#define CPX_SPI_MTU (CPX_SPI_MAX_PACKET_LENGTH - sizeof(cpx_spi_header_t))

// Send requests are split in two parts to allow senders to minimize the number of memory copies.
// It's up to the sender if and how to split, the protocol just sends the two parts concatenated.
typedef struct cpx_spi_send_req_s {
    // Internal cpx_spi over-the-wire header
    cpx_spi_header_t header;

    // Pointer to a buffer with the payload head, must be 4-byte aligned
    uint8_t *payload_head;

    // Pointer to a buffer with the payload tail, must be 4-byte aligned
    uint8_t *payload_tail;

    // Lengths of the two buffers
    uint16_t head_length;
    uint16_t tail_length;
} cpx_spi_send_req_t;

void cpx_spi_send_req_init(cpx_spi_send_req_t *req, uint8_t *payload_head, uint16_t head_length, uint8_t *payload_tail, uint16_t tail_length);
void cpx_spi_send_set_head(cpx_spi_send_req_t *req, uint8_t *payload_head, uint16_t head_length);
uint16_t cpx_spi_send_max_tail_length(cpx_spi_send_req_t *req);
void cpx_spi_send_set_tail(cpx_spi_send_req_t *req, uint8_t *payload_tail, uint16_t tail_length);

typedef struct cpx_spi_receive_req_s {
    // Internal cpx_spi over-the-wire header
    cpx_spi_header_t header;

    // Pointer to a buffer with the packet's payload, the buffer must also be 4-byte aligned
    uint8_t *buffer;

    // Size of the receive buffer. Since transfers on the wire always consist of simultaenous 
    // sends and receives, it must be as big as the maximum SENT packet in addition to the 
    // maximum received packet (i.e., equal to CPX_SPI_MTU).
    uint16_t buffer_size;
} cpx_spi_receive_req_t;

void cpx_spi_receive_req_init(cpx_spi_receive_req_t *req);

typedef struct cpx_spi_s {
    co_fn_ctx_t cpx_spi_ctx;

    pi_device_t spi;
    pi_device_t gpio;

    co_event_group_t events;

    pi_task_t nina_rtt_task;
    co_event_t spi_done;

    cpx_spi_send_req_t *send_req;
    pi_task_t *send_done;

    cpx_spi_receive_req_t *receive_req;
    pi_task_t *receive_done;

    // PMSIS limitations sometimes force us to make transfers even if they would not be needed, 
    // allocate an all-zeros cpx_spi_header (which is also the smallest supported size for a SPI transfer)
    cpx_spi_header_t empty_header;
} cpx_spi_t;

void cpx_spi_init(cpx_spi_t *cpx_spi);
void cpx_spi_start(cpx_spi_t *cpx_spi);

// These functions are not protected by any synchronization, care must be taken that each is
// ever called from one task at a time, waiting until the previous transfer completes before 
// calling them again. Sending and receiving can happen at the same time by two distinct tasks.
void cpx_spi_send_async(cpx_spi_t *cpx_spi, cpx_spi_send_req_t *req, pi_task_t *done_task);
void cpx_spi_receive_async(cpx_spi_t *cpx_spi, cpx_spi_receive_req_t *req, pi_task_t *done_task);

#endif // __CPX_SPI_H__
