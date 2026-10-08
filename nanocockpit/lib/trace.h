/*
 * trace.h
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
 * GPIO TRACING
 *
 * Low-overhead event tracing over GPIOs that can be recorded with a logic analyzer.
 *
 * Usable GPIOs on AI-deck:
 *  - GPIO_LED
 *  - GPIO_UART_TX
 *  - GPIO_I2C_SDA
 *  - GPIO_I2C_SCL
 *
 * The user is responsible of ensuring that the same GPIO is not already used
 * for another purpose
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include "config.h"

#include <pmsis.h>

#include <stdbool.h>

#define TRACE_GPIO_DISABLE -1

#define TRACE_CAMERA_BUF_0      GPIO_LED
#define TRACE_CAMERA_BUF_1      TRACE_GPIO_DISABLE
#define TRACE_CAMERA_CROP       TRACE_GPIO_DISABLE

#define TRACE_UART_PROTO_READ    TRACE_GPIO_DISABLE
#define TRACE_UART_PROTO_RESYNC  TRACE_GPIO_DISABLE
#define TRACE_UART_PROTO_CHKFAIL TRACE_GPIO_DISABLE
#define TRACE_UART_PROTO_MESSAGE TRACE_GPIO_DISABLE

#define TRACE_CPX_SEND          TRACE_GPIO_DISABLE
#define TRACE_CPX_RECEIVE       TRACE_GPIO_DISABLE
#define TRACE_CPX_SPI_TRANSFER  TRACE_GPIO_DISABLE
#define TRACE_CPX_SPI_WAIT_RTT  TRACE_GPIO_DISABLE

#define TRACE_STREAMER_SEND     TRACE_GPIO_DISABLE
#define TRACE_STREAMER_RECEIVE  TRACE_GPIO_DISABLE

#define TRACE_USER_0            TRACE_GPIO_DISABLE
#define TRACE_USER_1            TRACE_GPIO_DISABLE
#define TRACE_USER_2            TRACE_GPIO_DISABLE
#define TRACE_USER_3            TRACE_GPIO_DISABLE

void trace_init();
static inline void trace_set(int trace_id, bool state);

// Implementation

extern pi_device_t gpio;

static inline void trace_set(int trace_id, bool state) {
    if (trace_id == TRACE_GPIO_DISABLE) {
        return;
    }

    pi_gpio_pin_write(&gpio, trace_id, state);
}

#endif // __TRACE_H__
