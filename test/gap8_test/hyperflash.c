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
#include "stdio.h"
#include <bsp/flash/hyperflash.h>


#define HYPER_BUFF_SIZE 1024



int test_hyperflash()
{
  int errors = 0;
  struct pi_hyperflash_conf conf;
  char *buff;
  struct pi_device hyper;
  struct pi_flash_info flash_info;

  // Work-around for HW bug, this alternate has been set to 0 to have uart working, set it back for hyper
  pi_pad_set_function(PI_PAD_46_B7_SPIM0_SCK, PI_PAD_46_B7_HYPER_DQ6_FUNC3);

  printf("[HYPERFLASH] Start\n");

  pi_hyperflash_conf_init(&conf);

  pi_open_from_conf(&hyper, &conf);

  buff = pi_l2_malloc(HYPER_BUFF_SIZE);
  if (buff == NULL)
  {
    printf("[HYPERFLASH] Failed to allocate buffer\n");
    errors++;
    goto end0;
  }

  for (int i=0; i<HYPER_BUFF_SIZE; i++)
  {
    buff[i] = i;
  }

  if (pi_flash_open(&hyper))
  {
    printf("[HYPERFLASH] Failed to open ram\n");
    errors++;
    goto end1;
  }

  pi_flash_ioctl(&hyper, PI_FLASH_IOCTL_INFO, (void *)&flash_info);

  uint32_t flash_addr = ((flash_info.flash_start + flash_info.sector_size - 1) & ~(flash_info.sector_size - 1));

  pi_flash_erase(&hyper, flash_addr, flash_info.sector_size);

  pi_flash_program(&hyper, flash_addr, buff, HYPER_BUFF_SIZE);

  memset(buff, 0, HYPER_BUFF_SIZE);

  pi_flash_read(&hyper, flash_addr, buff, HYPER_BUFF_SIZE);

  for (int i=0; i<HYPER_BUFF_SIZE; i++)
  {
    unsigned char expected = i & 0xff;

    if (expected != buff[i])
    {
      errors++;
    }
  }

  if (errors)
    printf("[HYPERFLASH] Didn't get expected buffer\n");

  pi_flash_close(&hyper);

end1:
  pi_l2_free(buff, HYPER_BUFF_SIZE);

end0:

  if (errors)
    printf("[HYPERFLASH] Failed\n");
  else
    printf("[HYPERFLASH] Passed\n");

  return errors;
}
