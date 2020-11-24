/*
 * Copyright (C) 2018 GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pmsis.h"
#include "bsp/camera/himax.h"
#include "stdio.h"

// Himax camera is having 324x244 images
#define CAM_WIDTH    324
#define CAM_HEIGHT   244

int test_himax(void)
{
  struct pi_device device;
  unsigned char *imgBuff0;
  int errors = 0;

  printf("[CAMERA] Start\n");

  imgBuff0 = (unsigned char *)pi_l2_malloc((CAM_WIDTH*CAM_HEIGHT)*sizeof(unsigned char));
  if (imgBuff0 == NULL)
  {
    printf("[CAMERA] Failed to allocate memory for image\n");
    errors++;
    goto end0;
  }

  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  cam_conf.i2c_itf = 0;

  pi_open_from_conf(&device, &cam_conf);

  if (pi_camera_open(&device))
  {
    printf("[CAMERA] Failed to open camera driver\n");
    errors++;
    goto end;
  }

  uint8_t value = 0x01;
  pi_camera_reg_set(&device, 0x0601, &value);

  pi_camera_control(&device, PI_CAMERA_CMD_START, 0);

  pi_camera_capture(&device, imgBuff0, CAM_WIDTH*CAM_HEIGHT);

  pi_camera_control(&device, PI_CAMERA_CMD_STOP, 0);

  pi_camera_close(&device);

  for (int i=2; i<CAM_WIDTH-2; i+=2)
  {
    uint32_t expected;

    if (i < 52)
      expected = 0x00000000;
    else if (i < 106)
      expected = 0x000000ff;
    else if (i < 160)
      expected = 0xff000000;
    else if (i < 214)
      expected = 0xff0000ff;
    else if (i < 278)
      expected = 0x00ffff00;
    else
      expected = 0x00ffffff;

    for (int j=2; j<CAM_HEIGHT-2; j+=2)
    {
      uint32_t pattern = imgBuff0[j*CAM_WIDTH+i] | (imgBuff0[j*CAM_WIDTH+i+1] << 8) | (imgBuff0[(j+1)*CAM_WIDTH+i] << 16) | (imgBuff0[(j+1)*CAM_WIDTH+i+1] << 24);

      if (expected != pattern)
      {
        errors++;
        //printf("(%d, %d) %8.8lx %8.8lx\n", i, j, pattern, expected);
      }
    }
  }

  if (errors)
    printf("[CAMERA] Didn't get expected image\n");

end:
  pi_l2_free(imgBuff0, (CAM_WIDTH*CAM_HEIGHT)*sizeof(unsigned char));

end0:
  if (errors)
    printf("[CAMERA] Failed\n");
  else
    printf("[CAMERA] Passed\n");

  return errors;
}
