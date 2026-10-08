/*
 * uart.h
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

#ifndef __UART_H__
#define __UART_H__

#include <pmsis.h>

typedef struct uart_s {
    pi_device_t device;
} uart_t;

void uart_init(uart_t *uart);
int uart_printf(const char *fmt, ...);

/**
 * \brief Read data from an UART asynchronously.
 *
 * This reads data from the specified UART asynchronously.
 * A task must be specified in order to specify how the caller should be
 * notified when the transfer is finished.
 * Note: buffer must be in L2 memory on GAP8.
 *
 * \param device         Pointer to device descriptor of the UART device.
 * \param buffer         Pointer to data buffer.
 * \param size           Size of data to copy in bytes.
 * \param done_task      Event task used to notify the end of transfer.
 *
 * \retval size          Size of data that will be copied in bytes.
 */
static inline uint32_t uart_read_async(uart_t *uart, void *buffer, uint32_t size, pi_task_t *done_task) {
    pi_uart_read_async(&uart->device, buffer, size, done_task);
    return size;
}

static inline uint32_t uart_write_async(uart_t *uart, void *buffer, uint32_t size, pi_task_t *done_task) {
    pi_uart_write_async(&uart->device, buffer, size, done_task);
    return size;
}

#endif // __UART_H__
