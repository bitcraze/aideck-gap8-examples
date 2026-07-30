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
#include "camera_pipeline.h"

#include "bsp/camera/himax.h"
#include "himax_timing.h"
#include "stream_config.h"

#define CAPTURE_DONE_BIT (1 << 0)
#define IMG_ORIENTATION 0x0101

static struct pi_device camera;
static pi_task_t capture_tasks[CAPTURE_BUFFER_COUNT];
static EventGroupHandle_t capture_events;
static uint8_t *image_data[CAPTURE_BUFFER_COUNT];
static pi_buffer_t image_buffers[CAPTURE_BUFFER_COUNT];
static uint32_t capture_started_at[CAPTURE_BUFFER_COUNT];
static volatile uint32_t capture_duration;
static volatile uint32_t ready_buffer;
static uint32_t processing_buffer;
static volatile int consumer_waiting;
static int camera_streaming;
static int pipeline_primed;
static volatile uint32_t pending_captures;
static volatile int discarding_captures;

static void schedule_capture(uint32_t buffer_index);

static void capture_done(void *arg)
{
  uint32_t buffer_index = (uint32_t)(uintptr_t)arg;
  if (pending_captures > 0)
  {
    pending_captures--;
  }
  if (discarding_captures)
  {
    if (pending_captures == 0)
    {
      xEventGroupSetBits(capture_events, CAPTURE_DONE_BIT);
    }
    return;
  }

  if (consumer_waiting)
  {
    capture_duration =
      xTaskGetTickCount() - capture_started_at[buffer_index];
    ready_buffer = buffer_index;
    consumer_waiting = 0;
    xEventGroupSetBits(capture_events, CAPTURE_DONE_BIT);
  }
  else
  {
    schedule_capture(buffer_index);
  }
}

static void schedule_capture(uint32_t buffer_index)
{
  capture_started_at[buffer_index] = xTaskGetTickCount();
  pending_captures++;
  pi_camera_capture_async(
    &camera,
    image_data[buffer_index],
    CAMERA_WIDTH * CAMERA_HEIGHT,
    pi_task_callback(&capture_tasks[buffer_index], capture_done,
                     (void *)(uintptr_t)buffer_index));
}

static void wait_for_ready_frame(void)
{
  xEventGroupWaitBits(capture_events, CAPTURE_DONE_BIT, pdTRUE, pdFALSE,
                      (TickType_t)portMAX_DELAY);
}

static int open_camera(void)
{
  struct pi_himax_conf config;
  pi_himax_conf_init(&config);
  config.format = CAMERA_FORMAT;
  pi_open_from_conf(&camera, &config);
  if (pi_camera_open(&camera))
  {
    return -1;
  }

  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  uint8_t orientation = 3;
  uint8_t actual = 0;
  pi_camera_reg_set(&camera, IMG_ORIENTATION, &orientation);
  pi_time_wait_us(1000000);
  pi_camera_reg_get(&camera, IMG_ORIENTATION, &actual);
  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  if (actual != orientation)
  {
    return -1;
  }

#if SENSOR_FRAME_RATE_HZ > 0
  if (himax_configure_frame_timing(&camera, HIMAX_FRAME_LENGTH_LINES,
                                   HIMAX_MAX_INTEGRATION_LINES,
                                   HIMAX_QVGA_WINDOW_ENABLE))
  {
    return -1;
  }
#endif

  pi_camera_control(&camera, PI_CAMERA_CMD_AEG_INIT, 0);
  return 0;
}

int camera_pipeline_init(void)
{
  capture_events = xEventGroupCreate();
  if (capture_events == NULL)
  {
    return -1;
  }

  for (uint32_t i = 0; i < CAPTURE_BUFFER_COUNT; i++)
  {
    image_data[i] = pmsis_l2_malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
    if (image_data[i] == NULL)
    {
      return -1;
    }
    pi_buffer_init(&image_buffers[i], PI_BUFFER_TYPE_L2, image_data[i]);
    pi_buffer_set_format(&image_buffers[i], CAMERA_WIDTH, CAMERA_HEIGHT, 1,
                         PI_BUFFER_FORMAT_GRAY);
  }
  return open_camera();
}

CameraFrame_t camera_pipeline_begin_frame(void)
{
#if CAPTURE_MODE == CAPTURE_MODE_START_STOP
  consumer_waiting = 1;
  schedule_capture(processing_buffer);
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  wait_for_ready_frame();
  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  processing_buffer = ready_buffer;
#else
  if (!camera_streaming)
  {
    if (!pipeline_primed)
    {
      consumer_waiting = 1;
      schedule_capture(1);
      schedule_capture(2);
      pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
      wait_for_ready_frame();
      processing_buffer = ready_buffer;
      schedule_capture(0);
      pipeline_primed = 1;
    }
    else
    {
      pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    }
    camera_streaming = 1;
  }
#endif

  CameraFrame_t frame = {
    .data = image_data[processing_buffer],
    .buffer = &image_buffers[processing_buffer],
  };
  return frame;
}

uint32_t camera_pipeline_end_frame(void)
{
#if CAPTURE_MODE == CAPTURE_MODE_PIPELINED
  uint32_t wait_started_at = xTaskGetTickCount();
  consumer_waiting = 1;
  wait_for_ready_frame();
  schedule_capture(processing_buffer);
  processing_buffer = ready_buffer;
  return xTaskGetTickCount() - wait_started_at;
#else
  return 0;
#endif
}

void camera_pipeline_disconnect(void)
{
#if CAPTURE_MODE == CAPTURE_MODE_PIPELINED
  if (camera_streaming)
  {
    xEventGroupClearBits(capture_events, CAPTURE_DONE_BIT);
    discarding_captures = 1;
    /* Stop only after queued DMA captures finish at frame boundaries. */
    if (pending_captures > 0)
    {
      wait_for_ready_frame();
    }

    camera_streaming = 0;
    pipeline_primed = 0;
    consumer_waiting = 0;
    discarding_captures = 0;
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  }
#endif
}

uint32_t camera_pipeline_capture_time(void)
{
  return capture_duration;
}
