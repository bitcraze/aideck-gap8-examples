/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8
 *
 * Copyright (C) 2023 Bitcraze AB
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
 * Simple CPX communication with the Crazyflie, use together with the Crazyflie example app "app_stm_gap_cpx"
 */
#include "pmsis.h"
#include "bsp/bsp.h"
#include "cpx.h"

static CPXPacket_t rxPacket;
static CPXPacket_t txPacket;

void start_example(void) {
  pi_bsp_init();
  cpxInit();
  cpxEnableFunction(CPX_F_APP);

  cpxPrintToConsole(LOG_TO_CRTP, "Starting counter bouncer\n");

  while (1) {
    cpxReceivePacketBlocking(CPX_F_APP, &rxPacket);
    uint8_t counterInStm = rxPacket.data[0];

    // cpxPrintToConsole(LOG_TO_CRTP, "Got packet from the STM (%u)\n", counterInStm);

    // Bounce the same value back to the STM
    cpxInitRoute(CPX_T_GAP8, CPX_T_STM32, CPX_F_APP, &txPacket.route);
    txPacket.data[0] = counterInStm;
    txPacket.dataLength = 1;

    cpxSendPacketBlocking(&txPacket);
  }
}

int main(void) {
  return pmsis_kickoff((void *)start_example);
}
