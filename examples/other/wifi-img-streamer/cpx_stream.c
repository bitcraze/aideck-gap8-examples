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
#include "cpx_stream.h"

#include <string.h>

typedef struct
{
  uint8_t magic;
  uint16_t width;
  uint16_t height;
  uint8_t depth;
  uint8_t type;
  uint32_t size;
} __attribute__((packed)) ImageHeader_t;

static CPXPacket_t packet;

void cpx_stream_init(void)
{
  cpxInitRoute(CPX_T_GAP8, CPX_T_WIFI_HOST, CPX_F_APP, &packet.route);
}

void cpx_stream_send_header(uint16_t width, uint16_t height,
                            uint32_t image_size, StreamerMode_t image_type)
{
  ImageHeader_t *header = (ImageHeader_t *)packet.data;
  header->magic = 0xBC;
  header->width = width;
  header->height = height;
  header->depth = 1;
  header->type = image_type;
  header->size = image_size;
  packet.dataLength = sizeof(ImageHeader_t);
  cpxSendPacketBlocking(&packet);
}

void cpx_stream_send_buffer(const uint8_t *buffer, uint32_t buffer_size)
{
  uint32_t offset = 0;
  while (offset < buffer_size)
  {
    uint32_t chunk_size = sizeof(packet.data);
    if (offset + chunk_size > buffer_size)
    {
      chunk_size = buffer_size - offset;
    }
    memcpy(packet.data, buffer + offset, chunk_size);
    packet.dataLength = chunk_size;
    cpxSendPacketBlocking(&packet);
    offset += chunk_size;
  }
}
