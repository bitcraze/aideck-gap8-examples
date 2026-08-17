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

#include "bsp/camera/himax.h"

#ifndef CAMERA_FORMAT
#define CAMERA_FORMAT PI_CAMERA_QVGA
#endif

#ifndef CAMERA_WIDTH
#define CAMERA_WIDTH 324
#endif

#ifndef CAMERA_HEIGHT
#define CAMERA_HEIGHT 244
#endif

#define CAPTURE_MODE_START_STOP 0
#define CAPTURE_MODE_PIPELINED 2

#ifndef CAPTURE_MODE
#define CAPTURE_MODE CAPTURE_MODE_START_STOP
#endif

#ifndef OUTPUT_PROFILING_DATA
#define OUTPUT_PROFILING_DATA 0
#endif

#ifndef SENSOR_FRAME_RATE_HZ
#define SENSOR_FRAME_RATE_HZ 0
#endif

#ifndef STREAM_ENCODING_MODE
#define STREAM_ENCODING_MODE 0
#endif

#ifndef STREAM_WIDTH
#define STREAM_WIDTH 64
#endif

#ifndef STREAM_HEIGHT
#define STREAM_HEIGHT 48
#endif

#if STREAM_ENCODING_MODE == 2 && (STREAM_WIDTH == 0 || STREAM_HEIGHT == 0)
#error Stream dimensions must be greater than zero
#endif

#if STREAM_ENCODING_MODE == 2 && STREAM_WIDTH % 2 != 0
#error STREAM_WIDTH must be even for gray4 encoding
#endif

#if STREAM_ENCODING_MODE == 2 && \
    (STREAM_WIDTH > CAMERA_WIDTH || STREAM_HEIGHT > CAMERA_HEIGHT)
#error Stream dimensions must not exceed camera dimensions
#endif

#if CAPTURE_MODE == CAPTURE_MODE_PIPELINED
#define CAPTURE_BUFFER_COUNT 3
#else
#define CAPTURE_BUFFER_COUNT 1
#endif
