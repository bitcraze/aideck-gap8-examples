#pragma once

#include "bsp/buffer.h"
#include "pmsis.h"

typedef struct
{
  uint8_t *data;
  pi_buffer_t *buffer;
} CameraFrame_t;

int camera_pipeline_init(void);
CameraFrame_t camera_pipeline_begin_frame(void);
uint32_t camera_pipeline_end_frame(void);
void camera_pipeline_disconnect(void);
uint32_t camera_pipeline_capture_time(void);
