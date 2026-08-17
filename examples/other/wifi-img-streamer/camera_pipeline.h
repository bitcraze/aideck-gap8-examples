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

#include "bsp/buffer.h"
#include "pmsis.h"

typedef struct
{
  uint8_t *data;
  pi_buffer_t *buffer;
} CameraFrame_t;

typedef enum
{
  CAMERA_PIPELINE_OK = 0,
  CAMERA_PIPELINE_EVENT_ALLOC_FAILED,
  CAMERA_PIPELINE_BUFFER_ALLOC_FAILED,
  CAMERA_PIPELINE_CAMERA_OPEN_FAILED,
  CAMERA_PIPELINE_ORIENTATION_FAILED,
  CAMERA_PIPELINE_QVGA_WINDOW_FAILED,
  CAMERA_PIPELINE_FRAME_TIMING_FAILED,
  CAMERA_PIPELINE_FRAME_SYNC_FAILED,
} CameraPipelineStatus_t;

CameraPipelineStatus_t camera_pipeline_init(void);
const char *camera_pipeline_status_message(CameraPipelineStatus_t status);
CameraFrame_t camera_pipeline_begin_frame(void);
uint32_t camera_pipeline_end_frame(void);
void camera_pipeline_disconnect(void);
uint32_t camera_pipeline_queue_latency(void);
uint32_t camera_pipeline_dropped_frames(void);
