/*
 * Copyright (C) 2019 GreenWaves Technologies
 * Copyright (C) 2020 Bitcraze AB
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
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef __SPI_H__
#define __SPI_H__

#define CMD_PACKET_SIZE 16384

/* Initialize the SPI */
void spi_init();

/* Read data from the GAP8 SPI */
int spi_read_data(uint8_t ** buffer, size_t len);

#endif