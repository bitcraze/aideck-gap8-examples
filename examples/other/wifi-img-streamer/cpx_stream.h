/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ / /_/ / / / /_/  __/
 *    +------`   /_____/\_/\__/_/   \__,_/  /___/\___/
 *
 * AI-deck GAP8
 *
 * Copyright (C) 2026 Bitcraze AB
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
 * WiFi image streamer example
 */
#pragma once

#include "cpx.h"
#include "pmsis.h"

typedef enum
{
  RAW_ENCODING = 0,
  JPEG_ENCODING = 1,
  GRAY4_ENCODING = 2,
} __attribute__((packed)) StreamerMode_t;

void cpx_stream_init(void);
void cpx_stream_send_header(uint16_t width, uint16_t height,
                            uint32_t image_size, StreamerMode_t image_type);
void cpx_stream_send_buffer(const uint8_t *buffer, uint32_t buffer_size);
