/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8
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
 */
#include "pmsis.h"
#include "cpx.h"
#include "com.h"
#include "state.h"

typedef struct {
  CPXTarget_t destination : 3;
  CPXTarget_t source : 3;
  bool lastPacket : 1;
  bool reserved : 1;
  CPXFunction_t function : 6;
  uint8_t version : 2;
} __attribute__((packed)) CPXRoutingPacked_t;

typedef struct {
    uint16_t wireLength;
    CPXRoutingPacked_t route;
    uint8_t data[MTU - CPX_HEADER_SIZE];
} __attribute__((packed)) CPXPacketPacked_t;

#define QUEUE_LENGTH (2)
static xQueueHandle queues[CPX_F_LAST];

static CPXPacket_t rxp;
static CPXPacket_t txp;

static CPXPacketPacked_t rxpPacked;
static CPXPacketPacked_t txpPacked;

SemaphoreHandle_t xSemaphore = NULL;


static void cpx_rx_task(void *parameters) {
  while (1) {
    com_read((packet_t*)&rxpPacked);
    if (CPX_VERSION != rxpPacked.route.version)
    {
      set_state(STATE_ERROR);
    }
    configASSERT(CPX_VERSION == rxpPacked.route.version);
    rxp.route.version = rxpPacked.route.version;

    rxp.dataLength = rxpPacked.wireLength - CPX_HEADER_SIZE;
    rxp.route.destination = rxpPacked.route.destination;
    rxp.route.source = rxpPacked.route.source;
    rxp.route.function = rxpPacked.route.function;
    rxp.route.lastPacket = rxpPacked.route.lastPacket;
    memcpy(rxp.data, rxpPacked.data, rxp.dataLength);

    if (queues[rxp.route.function] != 0) {
      xQueueSend(queues[rxp.route.function], &rxp, portMAX_DELAY);
    } else {
      printf("No queue setup for function %d\n", rxp.route.function);
    }
  }
}

void cpxEnableFunction(CPXFunction_t function) {
  queues[function] = xQueueCreate(QUEUE_LENGTH, sizeof(CPXPacket_t));
  configASSERT(queues[function] != 0);
}

void cpxReceivePacketBlocking(CPXFunction_t function, CPXPacket_t * packet) {
  xQueueReceive(queues[function], packet, (TickType_t)portMAX_DELAY);
}

void cpxSendPacketBlocking(const CPXPacket_t * packet) {
  txpPacked.wireLength = packet->dataLength + CPX_HEADER_SIZE;
  txpPacked.route.destination = packet->route.destination;
  txpPacked.route.source = packet->route.source;
  txpPacked.route.function = packet->route.function;
  txpPacked.route.version = packet->route.version;
  txpPacked.route.lastPacket = packet->route.lastPacket;
  memcpy(txpPacked.data, packet->data, packet->dataLength);
  com_write((packet_t*)&txpPacked);
}

bool cpxSendPacket(const CPXPacket_t * packet, uint32_t timeout) {
  return true;
}

static CPXPacket_t consoleTx;
void cpxPrintToConsole(CPXConsoleTarget_t target, const char * fmt, ...) {
  if( xSemaphoreTake( xSemaphore, ( TickType_t )portMAX_DELAY) == pdTRUE )
  {
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf((char*)consoleTx.data, sizeof(consoleTx.data), fmt, ap);
    va_end(ap);

    consoleTx.route.destination = target;
    consoleTx.route.source = CPX_T_GAP8;
    consoleTx.route.function = CPX_F_CONSOLE;
    consoleTx.dataLength = len + 1;

    cpxSendPacketBlocking(&consoleTx);
    xSemaphoreGive( xSemaphore );
  }
}

void cpxInitRoute(const CPXTarget_t source, const CPXTarget_t destination, const CPXFunction_t function, CPXRouting_t* route) {
    route->source = source;
    route->destination = destination;
    route->function = function;
    route->version = CPX_VERSION;
    route->lastPacket = true;
}

void cpxInit(void) {

  com_init();

  memset(queues, CPX_F_LAST, sizeof(xQueueHandle));
  BaseType_t rxTask = xTaskCreate(cpx_rx_task, "rx_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (rxTask != pdPASS)
  {
    printf("Could not start router rx tasks!\n");
    pmsis_exit(-1);
  }
  xSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive( xSemaphore );
}
