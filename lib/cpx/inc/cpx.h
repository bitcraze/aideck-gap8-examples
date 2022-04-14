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
#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "com.h"

#define CPX_HEADER_SIZE (2)

// This enum is used to identify source and destination for CPX routing information
typedef enum {
  CPX_T_STM32 = 1, // The STM in the Crazyflie
  CPX_T_ESP32 = 2, // The ESP on the AI-deck
  CPX_T_HOST = 3,  // A remote computer connected via Wifi
  CPX_T_GAP8 = 4   // The GAP8 on the AI-deck
} CPXTarget_t;

typedef enum {
  CPX_F_SYSTEM = 1,
  CPX_F_CONSOLE = 2,
  CPX_F_CRTP = 3,
  CPX_F_WIFI_CTRL = 4,
  CPX_F_APP = 5,
  CPX_F_TEST = 0x0E,
  CPX_F_BOOTLOADER = 0x0F,
  CPX_F_LAST // NEEDS TO BE LAST
} CPXFunction_t;

typedef struct {
  CPXTarget_t destination;
  CPXTarget_t source;
  bool lastPacket;
  CPXFunction_t function;
} CPXRouting_t;

typedef struct {
  CPXRouting_t route;
  uint16_t dataLength;
  uint8_t data[MTU-CPX_HEADER_SIZE];
} CPXPacket_t;

/**
 * @brief Initialize the CPX module
 *
 */
void cpxInit(void);

/**
 * @brief Receive a CPX packet from the ESP32
 *
 * This function will block until a packet is availale from CPX. The
 * function will return all packets routed to the STM32.
 *
 * @param function function to receive packet on
 * @param packet received packet will be stored here
 */
void cpxReceivePacketBlocking(CPXFunction_t function, CPXPacket_t * packet);

/**
 * @brief Enable receiving on queue
 *
 * This will allocate data for a queue to be used when receiving
 * packets for a specific function.
 *
 * @param function packet to be sent
 */
void cpxEnableFunction( CPXFunction_t function);

/**
 * @brief Send a CPX packet to the ESP32
 *
 * This will send a packet to the ESP32 to be routed using CPX. This
 * will block until the packet can be queued up for sending.
 *
 * @param packet packet to be sent
 */
void cpxSendPacketBlocking(const CPXPacket_t * packet);

/**
 * @brief Send a CPX packet to the ESP32
 *
 * This will send a packet to the ESP32 to be routed using CPX.
 *
 * @param packet packet to be sent
 * @param timeout timeout before giving up if packet cannot be queued
 * @return true if package could be queued for sending
 * @return false if package could not be queued for sending within timeout
 */
bool cpxSendPacket(const CPXPacket_t * packet, uint32_t timeout);

/**
 * @brief Initialize CPX routing data.
 *
 * Initialize values and set lastPacket to true.
 *
 * @param source The starting point of the packet
 * @param destination The destination to send the packet to
 * @param function The function of the content
 * @param route Pointer to the route data to initialize
 */
void cpxInitRoute(const CPXTarget_t source, const CPXTarget_t destination, const CPXFunction_t function, CPXRouting_t* route);
