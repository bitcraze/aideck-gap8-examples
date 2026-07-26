#pragma once

#include "cpx.h"
#include "pmsis.h"

typedef enum
{
  RAW_ENCODING = 0,
  JPEG_ENCODING = 1,
} __attribute__((packed)) StreamerMode_t;

void cpx_stream_init(void);
void cpx_stream_send_header(uint16_t width, uint16_t height,
                            uint32_t image_size, StreamerMode_t image_type);
void cpx_stream_send_buffer(const uint8_t *buffer, uint32_t buffer_size);
