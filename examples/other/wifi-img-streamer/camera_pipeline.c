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
static uint32_t capture_queued_at[CAPTURE_BUFFER_COUNT];
static volatile uint32_t queue_latency;
static volatile uint32_t dropped_frames;
static volatile uint32_t ready_buffer;
static uint32_t processing_buffer;
static volatile int consumer_waiting;
static int camera_streaming;
static volatile uint32_t captures_queued;
static volatile uint32_t captures_completed;
static volatile int discarding_captures;

static void queue_capture(uint32_t buffer_index);
static void schedule_capture(uint32_t buffer_index);

static uint32_t outstanding_captures(void)
{
  return captures_queued - captures_completed;
}

static void capture_done(void *arg)
{
  uint32_t buffer_index = (uint32_t)(uintptr_t)arg;
  if (discarding_captures)
  {
    captures_completed++;
    if (outstanding_captures() == 0)
    {
      xEventGroupSetBits(capture_events, CAPTURE_DONE_BIT);
    }
    return;
  }

  if (consumer_waiting)
  {
    captures_completed++;
    queue_latency =
      xTaskGetTickCount() - capture_queued_at[buffer_index];
    ready_buffer = buffer_index;
    consumer_waiting = 0;
    xEventGroupSetBits(capture_events, CAPTURE_DONE_BIT);
  }
  else
  {
    dropped_frames++;
    /* Replacing a completed capture leaves the outstanding count unchanged. */
    queue_capture(buffer_index);
  }
}

static void queue_capture(uint32_t buffer_index)
{
  capture_queued_at[buffer_index] = xTaskGetTickCount();
  pi_camera_capture_async(
    &camera,
    image_data[buffer_index],
    CAMERA_WIDTH * CAMERA_HEIGHT,
    pi_task_callback(&capture_tasks[buffer_index], capture_done,
                     (void *)(uintptr_t)buffer_index));
}

static void schedule_capture(uint32_t buffer_index)
{
  captures_queued++;
  queue_capture(buffer_index);
}

static void wait_for_ready_frame(void)
{
  xEventGroupWaitBits(capture_events, CAPTURE_DONE_BIT, pdTRUE, pdFALSE,
                      (TickType_t)portMAX_DELAY);
}

static CameraPipelineStatus_t open_camera(void)
{
  struct pi_himax_conf config;
  pi_himax_conf_init(&config);
  config.format = CAMERA_FORMAT;
  pi_open_from_conf(&camera, &config);
  if (pi_camera_open(&camera))
  {
    return CAMERA_PIPELINE_CAMERA_OPEN_FAILED;
  }
  CameraPipelineStatus_t status = CAMERA_PIPELINE_OK;
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  uint8_t orientation = 3;
  uint8_t actual = 0;
  pi_camera_reg_set(&camera, IMG_ORIENTATION, &orientation);

#if SENSOR_FRAME_RATE_HZ > 0
  /* Apply the QVGA raster before its shorter frame timing becomes active. */
  if (himax_configure_qvga_window(&camera, HIMAX_QVGA_WINDOW_ENABLE))
  {
    status = CAMERA_PIPELINE_QVGA_WINDOW_FAILED;
  }
  if (status == CAMERA_PIPELINE_OK &&
      himax_wait_for_frames(&camera, 2, 1000000))
  {
    status = CAMERA_PIPELINE_FRAME_SYNC_FAILED;
  }
  if (status == CAMERA_PIPELINE_OK &&
      himax_configure_frame_timing(&camera, HIMAX_FRAME_LENGTH_LINES,
                                   HIMAX_MAX_INTEGRATION_LINES,
                                   HIMAX_QVGA_WINDOW_ENABLE))
  {
    status = CAMERA_PIPELINE_FRAME_TIMING_FAILED;
  }
#elif CAMERA_QVGA_WINDOW_ENABLE
  /* Keep the sensor's raster height equal to the QVGA DMA frame height. */
  if (himax_configure_qvga_window(&camera, 1))
  {
    status = CAMERA_PIPELINE_QVGA_WINDOW_FAILED;
  }
#else
  /* The GAP SDK leaves the HM01B0 register group held after initialization. */
  himax_commit_pending_registers(&camera);
#endif

  /* Prove the staged raster and orientation crossed frame boundaries. */
  if (status == CAMERA_PIPELINE_OK &&
      himax_wait_for_frames(&camera, 2, 1000000))
  {
    status = CAMERA_PIPELINE_FRAME_SYNC_FAILED;
  }
  pi_camera_reg_get(&camera, IMG_ORIENTATION, &actual);
  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  if (status != CAMERA_PIPELINE_OK)
  {
    return status;
  }
  if (actual != orientation)
  {
    return CAMERA_PIPELINE_ORIENTATION_FAILED;
  }

  pi_camera_control(&camera, PI_CAMERA_CMD_AEG_INIT, 0);
  return CAMERA_PIPELINE_OK;
}

CameraPipelineStatus_t camera_pipeline_init(void)
{
  capture_events = xEventGroupCreate();
  if (capture_events == NULL)
  {
    return CAMERA_PIPELINE_EVENT_ALLOC_FAILED;
  }

  for (uint32_t i = 0; i < CAPTURE_BUFFER_COUNT; i++)
  {
    image_data[i] = pmsis_l2_malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
    if (image_data[i] == NULL)
    {
      return CAMERA_PIPELINE_BUFFER_ALLOC_FAILED;
    }
    pi_buffer_init(&image_buffers[i], PI_BUFFER_TYPE_L2, image_data[i]);
    pi_buffer_set_format(&image_buffers[i], CAMERA_WIDTH, CAMERA_HEIGHT, 1,
                         PI_BUFFER_FORMAT_GRAY);
  }
  return open_camera();
}

const char *camera_pipeline_status_message(CameraPipelineStatus_t status)
{
  switch (status)
  {
    case CAMERA_PIPELINE_OK:
      return "camera pipeline initialized";
    case CAMERA_PIPELINE_EVENT_ALLOC_FAILED:
      return "failed to allocate capture event group";
    case CAMERA_PIPELINE_BUFFER_ALLOC_FAILED:
      return "failed to allocate camera buffers";
    case CAMERA_PIPELINE_CAMERA_OPEN_FAILED:
      return "failed to open camera";
    case CAMERA_PIPELINE_ORIENTATION_FAILED:
      return "camera orientation read-back failed";
    case CAMERA_PIPELINE_QVGA_WINDOW_FAILED:
      return "failed to configure QVGA sensor window";
    case CAMERA_PIPELINE_FRAME_TIMING_FAILED:
      return "failed to configure camera frame timing";
    case CAMERA_PIPELINE_FRAME_SYNC_FAILED:
      return "camera frame counter did not advance after configuration";
  }
  return "unknown camera pipeline error";
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
    consumer_waiting = 1;
    schedule_capture(1);
    schedule_capture(2);
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    wait_for_ready_frame();
    processing_buffer = ready_buffer;
    schedule_capture(0);
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
    wait_for_ready_frame();
    xEventGroupClearBits(capture_events, CAPTURE_DONE_BIT);

    camera_streaming = 0;
    consumer_waiting = 0;
    discarding_captures = 0;
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  }
#endif
}

uint32_t camera_pipeline_queue_latency(void)
{
  return queue_latency;
}

uint32_t camera_pipeline_dropped_frames(void)
{
  return dropped_frames;
}
