#pragma once

#include <stdint.h>
#include "com.h"

#include "cpx.h"

static packet_t txp;
static packet_t rxp;

// Return length of packet
uint32_t cpxReceivePacketBlocking(CPXPacket_t * packet) {
  uint32_t size;
  com_read(&rxp);
  size = rxp.len;
  memcpy(&packet->routing, rxp.data, size);
  return size;
}

void cpxSendPacketBlocking(CPXPacket_t * packet, uint32_t size) {
  uint32_t wireLength = size + sizeof(CPXRouting_t);
  txp.len = wireLength;
  memcpy(txp.data, &packet->routing, wireLength);
  com_write(&txp);
}

bool cpxSendPacket(CPXPacket_t * packet, uint32_t timeoutInMS) {
  // TODO
}