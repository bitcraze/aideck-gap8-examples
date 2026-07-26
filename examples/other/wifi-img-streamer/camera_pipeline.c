#include "camera_pipeline.h"

#include "bsp/camera/himax.h"
#include "himax_timing.h"
#include "stream_config.h"

#define CAPTURE_DONE_BIT (1 << 0)
#define IMG_ORIENTATION 0x0101

static struct pi_device camera;
static pi_task_t capture_task;
static EventGroupHandle_t capture_events;
static uint8_t *image_data[CAPTURE_BUFFER_COUNT];
static pi_buffer_t image_buffers[CAPTURE_BUFFER_COUNT];
static volatile uint32_t capture_done_at;
static uint32_t capture_duration;
static uint32_t pending_capture_at;
static uint32_t ready_buffer;
static uint32_t pending_buffer;
static int camera_streaming;
static int pipeline_primed;

static void capture_done(void *arg)
{
  (void)arg;
  capture_done_at = xTaskGetTickCount();
  xEventGroupSetBits(capture_events, CAPTURE_DONE_BIT);
}

static uint32_t schedule_capture(uint32_t buffer_index)
{
  uint32_t started_at = xTaskGetTickCount();
  pi_camera_capture_async(
    &camera,
    image_data[buffer_index],
    CAMERA_WIDTH * CAMERA_HEIGHT,
    pi_task_callback(&capture_task, capture_done, NULL));
  return started_at;
}

static void wait_for_capture(uint32_t started_at)
{
  xEventGroupWaitBits(capture_events, CAPTURE_DONE_BIT, pdTRUE, pdFALSE,
                      (TickType_t)portMAX_DELAY);
  capture_duration = capture_done_at - started_at;
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
  uint32_t started_at = schedule_capture(ready_buffer);
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  wait_for_capture(started_at);
  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
#else
  if (!camera_streaming)
  {
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    camera_streaming = 1;
  }

#if CAPTURE_MODE == CAPTURE_MODE_PIPELINED
  if (!pipeline_primed)
  {
    wait_for_capture(schedule_capture(ready_buffer));
    pipeline_primed = 1;
  }
  pending_buffer = ready_buffer ^ 1;
  pending_capture_at = schedule_capture(pending_buffer);
#else
  wait_for_capture(schedule_capture(ready_buffer));
#endif
#endif

  CameraFrame_t frame = {
    .data = image_data[ready_buffer],
    .buffer = &image_buffers[ready_buffer],
  };
  return frame;
}

uint32_t camera_pipeline_end_frame(void)
{
#if CAPTURE_MODE == CAPTURE_MODE_PIPELINED
  uint32_t wait_started_at = xTaskGetTickCount();
  wait_for_capture(pending_capture_at);
  ready_buffer = pending_buffer;
  return xTaskGetTickCount() - wait_started_at;
#else
  return 0;
#endif
}

void camera_pipeline_disconnect(void)
{
#if CAPTURE_MODE != CAPTURE_MODE_START_STOP
  if (camera_streaming)
  {
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    camera_streaming = 0;
    pipeline_primed = 0;
  }
#endif
}

uint32_t camera_pipeline_capture_time(void)
{
  return capture_duration;
}
