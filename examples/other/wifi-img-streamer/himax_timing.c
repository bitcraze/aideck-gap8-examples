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
#include "himax_timing.h"

#define HIMAX_FRAME_LENGTH_LINES_H 0x0340
#define HIMAX_MAX_INTEGRATION_LINES_H 0x2105
#define HIMAX_QVGA_WINDOW_ENABLE_REG 0x3010
#define HIMAX_GROUP_PARAMETER_HOLD 0x0104

static int set_register(struct pi_device *camera, uint32_t address,
                        uint8_t value)
{
  uint8_t actual = 0;
  pi_camera_reg_set(camera, address, &value);
  pi_camera_reg_get(camera, address, &actual);
  return actual == value ? 0 : -1;
}

static int set_register_pair(struct pi_device *camera, uint32_t high_address,
                             uint16_t value)
{
  uint8_t high = (value >> 8) & 0xff;
  uint8_t low = value & 0xff;
  return set_register(camera, high_address, high) ||
         set_register(camera, high_address + 1, low);
}

int himax_configure_frame_timing(struct pi_device *camera,
                                 uint16_t frame_length_lines,
                                 uint16_t max_integration_lines,
                                 uint8_t qvga_window_enable)
{
  if (set_register(camera, HIMAX_GROUP_PARAMETER_HOLD, 1))
  {
    return -1;
  }

  int failed = set_register_pair(camera, HIMAX_FRAME_LENGTH_LINES_H,
                                 frame_length_lines) ||
               set_register_pair(camera, HIMAX_MAX_INTEGRATION_LINES_H,
                                 max_integration_lines) ||
               set_register(camera, HIMAX_QVGA_WINDOW_ENABLE_REG,
                            qvga_window_enable);

  if (set_register(camera, HIMAX_GROUP_PARAMETER_HOLD, 0))
  {
    return -1;
  }
  return failed ? -1 : 0;
}
