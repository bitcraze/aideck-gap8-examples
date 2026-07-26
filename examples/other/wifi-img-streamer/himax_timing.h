#pragma once

#include "bsp/camera.h"
#include "pmsis.h"

int himax_configure_frame_timing(struct pi_device *camera,
                                 uint16_t frame_length_lines,
                                 uint16_t max_integration_lines,
                                 uint8_t qvga_window_enable);
