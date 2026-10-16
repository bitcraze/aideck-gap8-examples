/*
 * uart.c
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

#include "uart.h"

#include "debug.h"
#include "utils.h"

#include <pmsis.h>

#include <stdbool.h>
#include <stdarg.h>

static uart_t *_uart;

void uart_init(uart_t *uart) {
    struct pi_uart_conf uart_conf = {0};

    pi_uart_conf_init(&uart_conf);
    uart_conf.baudrate_bps = 115200;
    uart_conf.enable_tx = 1;
    uart_conf.enable_rx = 1;

    pi_open_from_conf(&uart->device, &uart_conf);

    int32_t status = pi_uart_open(&uart->device);

    VERBOSE_PRINT("UART init:\t\t\t%s\n", status ? "Failed" : "OK");

    if (status) {
        pmsis_exit(status);
    }
}

// extern int _prf(int (*func)(), void *dest, const char *format, va_list vargs);

// static int uart_tfp_putc(int c, void *dest) {
//   pi_uart_write(&uart, (uint8_t *)&c, 1);
//   return c;
// }

// int uart_printf(const char *fmt, ...) {
//   va_list va;
//   va_start(va, fmt);
//   _prf(uart_tfp_putc, NULL, fmt, va);
//   va_end(va);
//   return 0;
// }

#define PRINT_BUFFER_SIZE 128u
static char print_buffer[PRINT_BUFFER_SIZE];
static pi_task_t print_task;
static bool print_started = false;

extern int vsnprintf(char *s, size_t len, const char *format, va_list vargs);

int uart_printf(const char *fmt, ...) {
    if (print_started) {
        pi_task_wait_on(&print_task);
    }

    va_list va;
    va_start(va, fmt);
    size_t full_size = vsnprintf(print_buffer, PRINT_BUFFER_SIZE, fmt, va);
    va_end(va);
    
    // vsnprintf returns the buffer size that would be needed to fully print 
    // the string, not counting the '\0'. Compute the actually printed size.
    size_t size = MIN(full_size, PRINT_BUFFER_SIZE);
    pi_uart_write_async(&_uart->device, print_buffer, size, pi_task_block(&print_task));
    print_started = true;

    return size;
}
