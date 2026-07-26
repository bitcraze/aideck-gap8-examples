#pragma once

#include "pmsis.h"

int gray4_stream_init(void);
uint32_t gray4_stream_encode(const uint8_t *source);
const uint8_t *gray4_stream_data(void);
uint32_t gray4_stream_size(void);
