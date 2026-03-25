/*
 * cpx_spi.c
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

#include "cpx_spi.h"

#include "config.h"
#include "debug.h"
#include "trace.h"

#include <pmsis.h>

#include <stdint.h>

#ifdef CPX_SPI_VERBOSE
    #define SPI_VERBOSE_PRINT(...) CO_PRINT(__VA_ARGS__)
#else
    #define SPI_VERBOSE_PRINT(...) 
#endif

#ifdef CPX_SPI_BIDIRECTIONAL
    // Baudrate is limited to <7.2MHz if communicating GAP8<-ESP32 because of an AI-deck PCB bug
    // (see also src/nina/main/spi.c)
    #define CPX_SPI_BAUDRATE     7200000
#else
    #define CPX_SPI_BAUDRATE    30000000
#endif

#define CPX_SPI_ITF         (1)     // SPI1
#define CPX_SPI_CS          (0)     // CS0

enum cpx_spi_events_e {
    CPX_SPI_EVENT_NINA_RTT = 1 << 0,
    CPX_SPI_EVENT_SEND     = 1 << 1,
    CPX_SPI_EVENT_RECEIVE  = 1 << 2,

    CPX_SPI_EVENTS_ALL     = CPX_SPI_EVENT_NINA_RTT | CPX_SPI_EVENT_SEND | CPX_SPI_EVENT_RECEIVE,
};

CO_FN_DECLARE(cpx_spi_task);
static void cpx_spi_transfer_header_async(
    cpx_spi_t *cpx_spi, cpx_spi_send_req_t *send_req, cpx_spi_receive_req_t *receive_req, pi_task_t *done_task
);
static void cpx_spi_transfer_payload_head_async(
    cpx_spi_t *cpx_spi, cpx_spi_send_req_t *send_req, cpx_spi_receive_req_t *receive_req, pi_task_t *done_task
);
static void cpx_spi_transfer_payload_tail_async(
    cpx_spi_t *cpx_spi, cpx_spi_send_req_t *send_req, cpx_spi_receive_req_t *receive_req, pi_task_t *done_task
);

void cpx_spi_send_req_init(cpx_spi_send_req_t *req, uint8_t *payload_head, uint16_t head_length, uint8_t *payload_tail, uint16_t tail_length) {
    *req = (cpx_spi_send_req_t){0};
    cpx_spi_send_set_head(req, payload_head, head_length);
    cpx_spi_send_set_tail(req, payload_tail, tail_length);
}

static void cpx_spi_send_compute_length(cpx_spi_send_req_t *req) {
    size_t packet_length = req->head_length + req->tail_length;
    if (packet_length > CPX_SPI_MTU) {
        CO_ASSERTION_FAILURE(
            "Packet length (%d + %d bytes) exceeds max supported length of %d bytes (CPX_SPI_MTU).\n",
            req->head_length, req->tail_length, CPX_SPI_MTU
        );
    }
    req->header.length = packet_length;
}

void cpx_spi_send_set_head(cpx_spi_send_req_t *req, uint8_t *payload_head, uint16_t head_length) {
    if ((uintptr_t)payload_head % 4 != 0) {
        CO_ASSERTION_FAILURE("payload_head %p is not 4-byte aligned\n", payload_head);
    }

    if (head_length % 4 != 0) {
        CO_ASSERTION_FAILURE("head_length %d is not a multiple of 4 bytes\n", head_length);
    }
    
    req->payload_head = payload_head;
    req->head_length = head_length;

    cpx_spi_send_compute_length(req);
}

// Return the maximum supported tail length given the current head length and the need to keep the length multiple of 4 bytes
uint16_t cpx_spi_send_max_tail_length(cpx_spi_send_req_t *req) {
    size_t tail_length = CPX_SPI_MTU - req->head_length;

    if (tail_length % 4 != 0) {
        tail_length -= (tail_length % 4);
    }

    return tail_length;
}

void cpx_spi_send_set_tail(cpx_spi_send_req_t *req, uint8_t *payload_tail, uint16_t tail_length) {
    if ((uintptr_t)payload_tail % 4 != 0) {
        CO_ASSERTION_FAILURE("payload_tail %p is not 4-byte aligned\n", payload_tail);
    }

    if (tail_length % 4 != 0) {
        CO_ASSERTION_FAILURE("tail_length %d is not a multiple of 4 bytes\n", tail_length);
    }

    req->payload_tail = payload_tail;
    req->tail_length = tail_length;

    cpx_spi_send_compute_length(req);
}

void cpx_spi_receive_req_init(cpx_spi_receive_req_t *req) {
    req->buffer = pi_l2_malloc(CPX_SPI_MTU);
    req->buffer_size = CPX_SPI_MTU;

    if (!req->buffer) {
        CO_ASSERTION_FAILURE("Could not allocate cpx_spi_receive_req_t.\n");
    }

    memset(req->buffer, 0x77, CPX_SPI_MTU);
}

static void spi_init(cpx_spi_t *cpx_spi) {
    struct pi_spi_conf spi_conf = {0};

    pi_spi_conf_init(&spi_conf);
    spi_conf.wordsize = PI_SPI_WORDSIZE_8;
    spi_conf.big_endian = 1;
    spi_conf.max_baudrate = CPX_SPI_BAUDRATE;
    spi_conf.polarity = 0;
    spi_conf.phase = 0;
    spi_conf.itf = CPX_SPI_ITF;
    spi_conf.cs = CPX_SPI_CS;

    pi_open_from_conf(&cpx_spi->spi, &spi_conf);

    int32_t status = pi_spi_open(&cpx_spi->spi);

    VERBOSE_PRINT(
        "CPX SPI init:\t\t\t%s, "
#ifdef CPX_SPI_BIDIRECTIONAL
        "GAP8<=>ESP32"
#else
        "GAP8->ESP32"
#endif
        " @ %.1fMHz\n",
        status ? "Failed" : "OK",
        CPX_SPI_BAUDRATE / 1e6
    );

    if (status) {
        pmsis_exit(status);
    }
}

static void nina_rtt_callback(void *arg) {
    cpx_spi_t *cpx_spi = (cpx_spi_t *)arg;
    co_event_group_set(&cpx_spi->events, CPX_SPI_EVENT_NINA_RTT);
}

static pi_task_t *nina_rtt_event_init(cpx_spi_t *cpx_spi) {
    return pi_task_callback(&cpx_spi->nina_rtt_task, nina_rtt_callback, (void *)cpx_spi);
}

static void gap8_rtt_set(cpx_spi_t *cpx_spi, bool value) {
    pi_gpio_pin_write(&cpx_spi->gpio, GPIO_GAP8_RTT, value);
}

static bool nina_rtt_get(cpx_spi_t *cpx_spi) {
    uint32_t value;
    pi_gpio_pin_read(&cpx_spi->gpio, GPIO_NINA_RTT, &value);
    return (bool)value;
}

static void rtt_pins_init(cpx_spi_t *cpx_spi) {
    // FIXME: open the GPIO device a single time for the entire program
    struct pi_gpio_conf gpio_conf = {0};
    pi_gpio_conf_init(&gpio_conf);
    pi_open_from_conf(&cpx_spi->gpio, &gpio_conf);
    pi_gpio_open(&cpx_spi->gpio);

    // GAP8 READY-TO-TRANSMIT
    pi_gpio_pin_configure(&cpx_spi->gpio, GPIO_GAP8_RTT, PI_GPIO_OUTPUT);
    gap8_rtt_set(cpx_spi, false);

    // NINA READY-TO-TRANSMIT
    pi_gpio_pin_configure(&cpx_spi->gpio, GPIO_NINA_RTT, PI_GPIO_INPUT);

    // Ensure the callback is called immediately if RTT is already set
    if (nina_rtt_get(cpx_spi) == true) {
        nina_rtt_callback(cpx_spi);
    }

    // Setup a callback on NINA RTT rising edge
    pi_gpio_pin_task_add(&cpx_spi->gpio, GPIO_NINA_RTT, nina_rtt_event_init(cpx_spi), PI_GPIO_NOTIF_RISE);
}

void cpx_spi_init(cpx_spi_t *cpx_spi) {
    co_event_group_init(&cpx_spi->events);

    cpx_spi->receive_req = NULL;
    cpx_spi->receive_done = NULL;

    cpx_spi->send_req = NULL;
    cpx_spi->send_done = NULL;

    cpx_spi->empty_header = (cpx_spi_header_t){0};

    spi_init(cpx_spi);
    rtt_pins_init(cpx_spi);
}

void cpx_spi_start(cpx_spi_t *cpx_spi) {
    co_fn_push_start(&cpx_spi->cpx_spi_ctx, cpx_spi_task, (void *)cpx_spi, NULL);
}

void cpx_spi_send_async(cpx_spi_t *cpx_spi, cpx_spi_send_req_t *req, pi_task_t *done_task) {
    if (cpx_spi->send_req != NULL) {
        CO_ASSERTION_FAILURE("Multiple send requests in progress, not implemented!\n");
    }

    cpx_spi->send_req = req;
    cpx_spi->send_done = done_task;
    co_event_group_set(&cpx_spi->events, CPX_SPI_EVENT_SEND);
}

void cpx_spi_receive_async(cpx_spi_t *cpx_spi, cpx_spi_receive_req_t *req, pi_task_t *done_task) {
    if (cpx_spi->receive_req != NULL) {
        CO_ASSERTION_FAILURE("Multiple receive requests in progress, not implemented!\n");
    }

    if (req->buffer_size != CPX_SPI_MTU) {
        // NOTE: the code depends on this also because transfers on the wire always consist of simultaenous sends and receives
        // The receive buffer must be as big as the maximum SENT packet in addition to the maximum received packet.
        CO_ASSERTION_FAILURE("Buffer must have enough space to contain every possible packet size (CPX_SPI_MTU)\n");
    }

    cpx_spi->receive_req = req;
    cpx_spi->receive_done = done_task;
    co_event_group_set(&cpx_spi->events, CPX_SPI_EVENT_RECEIVE);
}

CO_FN_BEGIN(cpx_spi_task, cpx_spi_t *, cpx_spi)
{
    static co_event_mask_t events;
    static cpx_spi_send_req_t *send_req;
    static cpx_spi_receive_req_t *receive_req;
    
    while (true) {
        // 1) Wait until someone wants to transmit data
        events = CPX_SPI_EVENT_NINA_RTT | CPX_SPI_EVENT_SEND;
        CO_WAIT_GROUP_ANY(&cpx_spi->events, &events);

        // 2) In any case, wait until we are also ready to receive
        // TODO: figure out if we can decouple transmit from receive
        events = CPX_SPI_EVENT_RECEIVE;
        CO_WAIT_GROUP_ALL(&cpx_spi->events, &events);

        // 3) Notify NINA if we have data to transmit
        events = co_event_group_get(&cpx_spi->events, CPX_SPI_EVENT_SEND);
        if (events & CPX_SPI_EVENT_SEND) {
            gap8_rtt_set(cpx_spi, true);
        }

        // 4) Ensure that NINA is ready to receive, in case we woke up due to CPX_SPI_EVENT_SEND at 1)
        trace_set(TRACE_CPX_SPI_WAIT_RTT, true);
        events = CPX_SPI_EVENT_NINA_RTT;
        CO_WAIT_GROUP_ALL(&cpx_spi->events, &events);
        trace_set(TRACE_CPX_SPI_WAIT_RTT, false);

        // 5) Everyone is ready for the transfer. Fetch all event bits together to get the final overall state
        // NOTE 1: at this point we should always have CPX_SPI_EVENT_NINA_RTT and CPX_SPI_EVENT_RECEIVE
        // NOTE 2: CPX_SPI_EVENT_SEND might have be set between 1) and here, here we are still in time to
        //         coalesce the send in the current transfer and save some time
        events = co_event_group_get(&cpx_spi->events, CPX_SPI_EVENTS_ALL);
        send_req = (events & CPX_SPI_EVENT_SEND) ? cpx_spi->send_req : NULL;
        receive_req = (events & CPX_SPI_EVENT_RECEIVE) ? cpx_spi->receive_req : NULL;

        trace_set(TRACE_CPX_SPI_TRANSFER, true);

        // 6) Transfer the cpx_spi_header_t
        cpx_spi_transfer_header_async(
            cpx_spi, send_req, receive_req, co_event_init(&cpx_spi->spi_done)
        );
        CO_WAIT(&cpx_spi->spi_done);
        
        // 7) Disable GAP8 RTT while the transfer is still ongoing to prevent race conditions
        gap8_rtt_set(cpx_spi, false);

        // 8) Re-arm the NINA RTT event while the transfer is still ongoing to be sure of catching the next rising edge
        co_event_group_clear(&cpx_spi->events, CPX_SPI_EVENT_NINA_RTT);
        nina_rtt_event_init(cpx_spi);

#ifndef CPX_SPI_BIDIRECTIONAL
        // Any received packet will be corrupted if bidirectional communication is disabled,
        // ensure it is ignored
        if (receive_req) {
            receive_req->header.length = 0;
        }
#endif

        // 9) Transfer the send_req's payload_head and an equivalent length of receive_req
        cpx_spi_transfer_payload_head_async(
            cpx_spi, send_req, receive_req, co_event_init(&cpx_spi->spi_done)
        );
        CO_WAIT(&cpx_spi->spi_done);

        // 10) Transfer the send_req's payload_tail and the remaining length of receive_req
        cpx_spi_transfer_payload_tail_async(
            cpx_spi, send_req, receive_req, co_event_init(&cpx_spi->spi_done)
        );
        CO_WAIT(&cpx_spi->spi_done);

        trace_set(TRACE_CPX_SPI_TRANSFER, false);

        // 11) Notify the sender that the send was completed
        if (send_req) {
            cpx_spi->send_req = NULL;
            pi_task_push(cpx_spi->send_done);
            co_event_group_clear(&cpx_spi->events, CPX_SPI_EVENT_SEND);
        }

        // 12) Notify the receiver that the receive was completed
        // NOTE: at the moment, receive_req should always be present
        if (receive_req) {
            cpx_spi->receive_req = NULL;
            pi_task_push(cpx_spi->receive_done);
            co_event_group_clear(&cpx_spi->events, CPX_SPI_EVENT_RECEIVE);
        }
    }
}
CO_FN_END()

static void spi_transfer_async(pi_device_t *spi, void *tx_data, void *rx_data, size_t length, pi_spi_flags_e flags, pi_task_t *done_task) {
    if ((uintptr_t)tx_data % 4 != 0) {
        CO_ASSERTION_FAILURE("tx_data is not 4-byte aligned\n");
    }

    if ((uintptr_t)rx_data % 4 != 0) {
        CO_ASSERTION_FAILURE("rx_data is not 4-byte aligned\n");
    }

    // Both GAP8 and ESP need the transfer length to be a multiple of 4 bytes
    if ((length / 8) % 4 != 0) {
        CO_ASSERTION_FAILURE("length (%d bits) is not a multiple of 4 bytes\n", length);
    }

    SPI_VERBOSE_PRINT("spi_transfer_async tx %p, rx %p, length %d, flags %d\n", tx_data, rx_data, length, flags);

    if (tx_data && rx_data) {
        pi_spi_transfer_async(spi, tx_data, rx_data, length, flags, done_task);
    } else if (tx_data) {
        pi_spi_send_async(spi, tx_data, length, flags, done_task);
    } else if (rx_data) {
        pi_spi_receive_async(spi, rx_data, length, flags, done_task);
    } else {
        CO_ASSERTION_FAILURE("No tx_data nor rx_data, what are you trying to transfer?\n");
    }
}

static void cpx_spi_transfer_header_async(cpx_spi_t *cpx_spi,
                                          cpx_spi_send_req_t *send_req, cpx_spi_receive_req_t *receive_req,
                                          pi_task_t *done_task) {
    size_t transfer_length = sizeof(cpx_spi_header_t);
    pi_spi_flags_e flags = PI_SPI_LINES_SINGLE | PI_SPI_CS_KEEP;
    spi_transfer_async(
        &cpx_spi->spi,
        send_req ? &send_req->header : &cpx_spi->empty_header,
        receive_req ? &receive_req->header : NULL,
        transfer_length * 8, flags, done_task
    );
}

static void cpx_spi_transfer_payload_head_async(cpx_spi_t *cpx_spi,
                                                cpx_spi_send_req_t *send_req, cpx_spi_receive_req_t *receive_req,
                                                pi_task_t *done_task) {
    uint16_t send_length = 0, head_length = 0;
    uint8_t *send_buffer = NULL;
    if (send_req) {
        send_length = send_req->header.length;
        head_length = send_req->head_length;
        send_buffer = send_req->payload_head;
    }

    // Ensure that received packet length cannot exceed CPX_SPI_MTU
    uint16_t receive_length = 0;
    uint8_t *receive_buffer = NULL;
    if (receive_req) {
        receive_length = MIN(receive_req->header.length, CPX_SPI_MTU);
        receive_buffer = receive_req->buffer;
    }

    // Total length of the transfer
    uint16_t total_length = MAX(send_length, receive_length);
    
    // Ignore the received packet for the moment, cpx_spi_transfer_payload_tail_async will 
    // complete reception if necessary. Note that head_length is checked in cpx_spi_send_req_init 
    // to be less than CPX_SPI_MTU, so it cannot overrun the receive buffer
    size_t transfer_length = head_length;

    // Check whether there will be data left to transfer for cpx_spi_transfer_payload_tail_async
    bool end_of_transfer = (total_length == transfer_length);
    pi_spi_flags_e flags = PI_SPI_LINES_SINGLE | (end_of_transfer ? PI_SPI_CS_AUTO : PI_SPI_CS_KEEP);

    // A sender may decide to have a zero-sized payload_head, but calling pi_spi_transfer_async with zero size 
    // would corrupt the following transaction.
    if (transfer_length == 0) {
        if (end_of_transfer) {
            // No more data to transfer, but need to clear the CS.
            // Send a dummy transfer that NINA will ignore because it is over the specified packet length.
            pi_spi_send_async(&cpx_spi->spi, &cpx_spi->empty_header, sizeof(cpx_spi_header_t) * 8, flags, done_task);
        } else {
            // There is still data to be transmitted in cpx_spi_transfer_payload_tail_async, we can just 
            // complete immediately
            pi_task_push(done_task);
        }
    } else {
        spi_transfer_async(&cpx_spi->spi, send_buffer, receive_buffer, transfer_length * 8, flags, done_task);
    }
}

static void cpx_spi_transfer_payload_tail_async(cpx_spi_t *cpx_spi,
                                                cpx_spi_send_req_t *send_req, cpx_spi_receive_req_t *receive_req,
                                                pi_task_t *done_task) {
    uint16_t send_length = 0, head_length = 0;
    uint8_t *send_buffer = NULL;
    if (send_req) {
        send_length = send_req->header.length;
        head_length = send_req->head_length;
        send_buffer = send_req->payload_tail;
    }

    // Ensure that received packet length cannot exceed CPX_SPI_MTU
    uint16_t receive_length = 0;
    uint8_t *receive_buffer = NULL;
    if (receive_req) {
        receive_length = MIN(receive_req->header.length, CPX_SPI_MTU);
        receive_buffer = receive_req->buffer + head_length;
    }

    // Total length of the transfer
    uint16_t total_length = MAX(send_length, receive_length);

    // FIXME: this causes an out-of-bounds read on send_req->payload_tail if tail_length < receive_req length
    // This should only be an information leak, since NINA then ignores this extra data because it is over the 
    // specified packet length.
    // We should be able to avoid this by introducing a fourth transfer phase where we only receive the 
    // remaining receive_req, but we need to take care to ensure that CS is handled correctly.
    size_t remaining_length = total_length - head_length;

    if (remaining_length == 0) {
        // No data left to transmit, cpx_spi_transfer_payload_head_async has already cleared the CS.
        // We can just complete immediately.
        pi_task_push(done_task);
        return;
    }

    // Round up the transfer length to a multiple of 4 bytes. NOTE: we ensure that head_length and
    // tail_length of sent packets are always multiples of 4 bytes, so this should only happen if 
    // receive_length > send_length and receive_length - head_length is not a multiple of 4. Assuming that 
    // CPX_SPI_MAX_PACKET_LENGTH is a multiple of 4 bytes, this will round up to CPX_SPI_MAX_PACKET_LENGTH at 
    // most, never causing any out-of-bounds writes on receive_buffer.
    if (remaining_length % 4 != 0) {
        remaining_length += (4 - remaining_length % 4);
    }

    pi_spi_flags_e flags = PI_SPI_LINES_SINGLE | PI_SPI_CS_AUTO;
    spi_transfer_async(&cpx_spi->spi, send_buffer, receive_buffer, remaining_length * 8, flags, done_task);
}
