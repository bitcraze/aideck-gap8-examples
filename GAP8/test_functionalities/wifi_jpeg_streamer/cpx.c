/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8 second stage bootloader
 *
 * Copyright (C) 2022 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * cpx.c - Interface for CPX stack
 */
#pragma once

#include <stdint.h>
#include "com.h"
#include "pmsis.h"
#include "cpx.h"

typedef struct
{
  uint16_t length; // Length data from cpxDst
  uint8_t cpxDst : 3;
  uint8_t cpxSrc : 3;
  bool lastPacket : 1;
  bool reserved : 1;
  uint8_t cpxFunc;
  uint8_t data[MTU - CPX_HEADER_SIZE];
} __attribute__((packed)) spi_transport_with_routing_packet_t;

static spi_transport_with_routing_packet_t txp;
static spi_transport_with_routing_packet_t rxp;

// Return length of packet
uint32_t cpxReceivePacketBlocking(CPXPacket_t * packet) {
  com_read((packet_t*) &rxp);
  uint32_t size = rxp.length - sizeof(CPXRouting_t);

  size = (uint32_t) rxp.length - CPX_HEADER_SIZE;
  packet->route.destination = rxp.cpxDst;
  packet->route.source = rxp.cpxSrc;
  packet->route.function = rxp.cpxFunc;
  memcpy(packet->data, rxp.data, size);

  return size;
}

static CPXPacket_t consoleTx;
void cpxPrintToConsole(CPXConsoleTarget_t target, const char * fmt, ...) {
  va_list ap;
  int len;

  va_start(ap, fmt);
  len = vsnprintf(consoleTx.data, sizeof(consoleTx.data), fmt, ap);
  va_end(ap);

  consoleTx.route.destination = target;
  consoleTx.route.source = GAP8;
  consoleTx.route.function = CONSOLE;

  cpxSendPacketBlocking(&consoleTx, len + 1);
}

void cpxSendPacketBlocking(CPXPacket_t * packet, uint32_t size) {
  txp.length = size + CPX_HEADER_SIZE;
  txp.cpxDst = packet->route.destination;
  txp.cpxSrc = packet->route.source;
  txp.cpxFunc = packet->route.function;
  memcpy(txp.data, &packet->data, size);

  com_write((packet_t*) &txp);
}
