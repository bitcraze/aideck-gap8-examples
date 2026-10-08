/*
 * trace.c
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

#include "trace.h"

#include "config.h"
#include "debug.h"

#include <pmsis.h>

#include <stdbool.h>

pi_device_t gpio;

static void trace_configure(int trace_id) {
    if (trace_id == TRACE_GPIO_DISABLE) {
        return;
    }
    
    pi_gpio_pin_configure(&gpio, trace_id, PI_GPIO_OUTPUT);
    pi_gpio_pin_write(&gpio, trace_id, false);
}

void trace_init() {
    trace_configure(TRACE_CAMERA_BUF_0);
    trace_configure(TRACE_CAMERA_BUF_1);
    trace_configure(TRACE_CAMERA_CROP);
    
    trace_configure(TRACE_UART_PROTO_READ);
    trace_configure(TRACE_UART_PROTO_RESYNC);
    trace_configure(TRACE_UART_PROTO_CHKFAIL);
    trace_configure(TRACE_UART_PROTO_MESSAGE);

    trace_configure(TRACE_CPX_SEND);
    trace_configure(TRACE_CPX_RECEIVE);
    trace_configure(TRACE_CPX_SPI_TRANSFER);
    trace_configure(TRACE_CPX_SPI_WAIT_RTT);

    trace_configure(TRACE_STREAMER_SEND);
    trace_configure(TRACE_STREAMER_RECEIVE);

    trace_configure(TRACE_USER_0);
    trace_configure(TRACE_USER_1);
    trace_configure(TRACE_USER_2);
    trace_configure(TRACE_USER_3);
    
    VERBOSE_PRINT(
        "Trace init:\t\t\tOK, (active: "
#if TRACE_CAMERA_BUF_0 != TRACE_GPIO_DISABLE
        "camera_buf_0, "
#endif
#if TRACE_CAMERA_BUF_1 != TRACE_GPIO_DISABLE
        "camera_buf_1, "
#endif
#if TRACE_CAMERA_CROP != TRACE_GPIO_DISABLE
        "camera_crop, "
#endif

#if TRACE_UART_PROTO_READ != TRACE_GPIO_DISABLE
        "uart_read, "
#endif
#if TRACE_UART_PROTO_RESYNC != TRACE_GPIO_DISABLE
        "uart_proto_resync, "
#endif
#if TRACE_UART_PROTO_CHKFAIL != TRACE_GPIO_DISABLE
        "uart_proto_chkfail, "
#endif
#if TRACE_UART_PROTO_MESSAGE != TRACE_GPIO_DISABLE
        "uart_proto_message, "
#endif

#if TRACE_CPX_SEND != TRACE_GPIO_DISABLE
        "cpx_send, "
#endif
#if TRACE_CPX_RECEIVE != TRACE_GPIO_DISABLE
        "cpx_receive, "
#endif
#if TRACE_CPX_SPI_TRANSFER != TRACE_GPIO_DISABLE
        "cpx_spi_transfer, "
#endif
#if TRACE_CPX_SPI_WAIT_RTT != TRACE_GPIO_DISABLE
        "cpx_spi_wait_rtt, "
#endif

#if TRACE_STREAMER_SEND != TRACE_GPIO_DISABLE
        "streamer_send, "
#endif
#if TRACE_STREAMER_RECEIVE != TRACE_GPIO_DISABLE
        "streamer_receive, "
#endif

#if TRACE_USER_0 != TRACE_GPIO_DISABLE
        "user_0, "
#endif
#if TRACE_USER_1 != TRACE_GPIO_DISABLE
        "user_1, "
#endif
#if TRACE_USER_2 != TRACE_GPIO_DISABLE
        "user_2, "
#endif
#if TRACE_USER_3 != TRACE_GPIO_DISABLE
        "user_3, "
#endif
        ")\n"
    );
}
