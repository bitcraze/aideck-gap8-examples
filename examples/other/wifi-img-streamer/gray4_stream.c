#include "gray4_stream.h"

#include "stream_config.h"

static uint8_t *packed_data;
static uint16_t x_map[STREAM_WIDTH];
static uint16_t y_map[STREAM_HEIGHT];

uint32_t gray4_stream_size(void)
{
  return ((uint32_t)STREAM_WIDTH * STREAM_HEIGHT + 1) / 2;
}

static void build_index_map(uint16_t *map, uint16_t output_size,
                            uint16_t input_size)
{
  for (uint16_t i = 0; i < output_size; i++)
  {
    map[i] = ((uint32_t)i * input_size) / output_size;
  }
}

int gray4_stream_init(void)
{
  packed_data = pmsis_l2_malloc(gray4_stream_size());
  if (packed_data == NULL)
  {
    return -1;
  }

  build_index_map(x_map, STREAM_WIDTH, CAMERA_WIDTH);
  build_index_map(y_map, STREAM_HEIGHT, CAMERA_HEIGHT);
  return 0;
}

uint32_t gray4_stream_encode(const uint8_t *source)
{
  uint32_t started_at = xTaskGetTickCount();
  uint32_t output_offset = 0;

  for (uint16_t y = 0; y < STREAM_HEIGHT; y++)
  {
    const uint8_t *row = source + (uint32_t)y_map[y] * CAMERA_WIDTH;
    for (uint16_t x = 0; x < STREAM_WIDTH; x += 2)
    {
      uint8_t packed = (row[x_map[x]] >> 4) << 4;
      if (x + 1 < STREAM_WIDTH)
      {
        packed |= row[x_map[x + 1]] >> 4;
      }
      packed_data[output_offset++] = packed;
    }
  }

  return xTaskGetTickCount() - started_at;
}

const uint8_t *gray4_stream_data(void)
{
  return packed_data;
}
