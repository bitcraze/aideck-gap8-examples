/*
 * uart_protocol.c
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

#include "uart_protocol.h"

#include "config.h"
#include "crc32.h"
#include "debug.h"
#include "time.h"
#include "trace.h"
#include "uart.h"

#include <pmsis.h>

#include <stdbool.h>

CO_FN_BEGIN(uart_protocol_task, uart_protocol_t *, protocol)
{
    static void *buffer;
    static uart_msg_t *message;
    static uint32_t available_length;
    static uint32_t discarded_length;
    static uint32_t message_length;
    static uint32_t total_length;

    trace_set(TRACE_UART_PROTO_RESYNC, false);

    while (true) {
        buffer = protocol->buffer;
        available_length = 0;
        discarded_length = 0;
        message_length = 0;
        
        trace_set(TRACE_UART_PROTO_READ, true);
        available_length += uart_read_async(protocol->uart, buffer + available_length, UART_READ_SIZE, co_event_init(&protocol->done_event));
        CO_WAIT(&protocol->done_event);
        trace_set(TRACE_UART_PROTO_READ, false);

        do {
            message = (uart_msg_t *)(protocol->buffer + discarded_length);

            if (memcmp(message->header, UART_STATE_MSG_HEADER, UART_HEADER_LENGTH) == 0) {
                message_length = sizeof(state_msg_t);
                break;
            } else if (memcmp(message->header, UART_RNG_MSG_HEADER, UART_HEADER_LENGTH) == 0) {
                message_length = sizeof(rng_msg_t);
                break;
            } else if (memcmp(message->header, UART_TOF_MSG_HEADER, UART_HEADER_LENGTH) == 0) {
                message_length = sizeof(tof_msg_t);
                break;
            }

            trace_set(TRACE_UART_PROTO_RESYNC, true);

            discarded_length += 1;

            if ((discarded_length + UART_READ_SIZE) > UART_BUFFER_LENGTH) {
                // Give up and start again from the beginning of the buffer
                break;
            }

            if ((discarded_length + UART_HEADER_LENGTH) > available_length) {
                trace_set(TRACE_UART_PROTO_READ, true);
                available_length += uart_read_async(protocol->uart, buffer + available_length, UART_READ_SIZE, co_event_init(&protocol->done_event));
                CO_WAIT(&protocol->done_event);
                trace_set(TRACE_UART_PROTO_READ, false);
            }
        } while (true);

        if (!message_length) {
            continue;
        }

        trace_set(TRACE_UART_PROTO_RESYNC, false);

        total_length = discarded_length + UART_HEADER_LENGTH + message_length + UART_CHECKSUM_LENGTH;

        if (total_length > UART_BUFFER_LENGTH) {
            // Give up and start again from the beginning of the buffer
            // FIXME: potentially move the received header to the beginning of the buffer instead of simply 
            // dropping a message when discarding too much
            continue;
        }

        int32_t remaining_length = total_length - available_length;
        if (remaining_length > 0) {
            trace_set(TRACE_UART_PROTO_READ, true);
            available_length += uart_read_async(protocol->uart, buffer + available_length, remaining_length, co_event_init(&protocol->done_event));
            CO_WAIT(&protocol->done_event);
            trace_set(TRACE_UART_PROTO_READ, false);
        }

        memmove(&message->checksum, (void *)message + UART_HEADER_LENGTH + message_length, sizeof(uint32_t));
        message->recv_timestamp = time_get_us();

        uint32_t checksum = crc32CalculateBuffer(message, UART_HEADER_LENGTH + message_length);
        if (checksum != message->checksum) {
            // VERBOSE_PRINT("Received UART message with header '%.4s' and invalid CRC %u (expected %u)\n", message->header, checksum, message->checksum);
            trace_set(TRACE_UART_PROTO_CHKFAIL, true);
            trace_set(TRACE_UART_PROTO_CHKFAIL, false);
            continue;
        }
        
        trace_set(TRACE_UART_PROTO_MESSAGE, true);
        co_fn_push_start(&protocol->message_ctx, protocol->message_callback, (void *)message, co_event_init(&protocol->done_event));
        CO_WAIT(&protocol->done_event);
        trace_set(TRACE_UART_PROTO_MESSAGE, false);

        if (available_length > total_length) {
            trace_set(TRACE_UART_PROTO_CHKFAIL, true);
            trace_set(TRACE_UART_PROTO_CHKFAIL, false);
            trace_set(TRACE_UART_PROTO_CHKFAIL, true);
            trace_set(TRACE_UART_PROTO_CHKFAIL, true);
            trace_set(TRACE_UART_PROTO_CHKFAIL, false);
        }
    }
}
CO_FN_END()

void uart_protocol_init(uart_protocol_t *protocol, uart_t* uart, co_fn_t message_callback) {
    protocol->uart = uart;
    protocol->message_callback = message_callback;
}

void uart_protocol_start(uart_protocol_t *protocol) {
    co_fn_push_start(&protocol->protocol_ctx, uart_protocol_task, (void *)protocol, NULL);
}

void uart_protocol_send_inference_async(uart_protocol_t *protocol, inference_stamped_msg_t *msg, pi_task_t *done_task) {
    uint32_t message_size = UART_HEADER_LENGTH + sizeof(inference_stamped_msg_t);
    memcpy(protocol->tx_message.header, UART_INFERENCE_STAMPED_MSG_HEADER, UART_HEADER_LENGTH);
    protocol->tx_message.inference_stamped = *msg;
    uart_write_async(protocol->uart, &protocol->tx_message, message_size, done_task);
}
