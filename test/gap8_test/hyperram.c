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
#include <bsp/ram/hyperram.h>


#define HYPER_BUFF_SIZE 1024



int test_hyperram()
{
  int errors = 0;
  struct pi_hyperram_conf conf;
  char *buff;
  uint32_t hyper_buff;
  struct pi_device hyper;

  // Work-around for HW bug, this alternate has been set to 0 to have uart working, set it back for hyper
  pi_pad_set_function(PI_PAD_46_B7_SPIM0_SCK, PI_PAD_46_B7_HYPER_DQ6_FUNC3);


  printf("[HYPERRAM] Start\n");

  pi_hyperram_conf_init(&conf);

  pi_open_from_conf(&hyper, &conf);

  buff = pi_l2_malloc(HYPER_BUFF_SIZE);
  if (buff == NULL)
  {
    printf("[HYPERRAM] Failed to allocate buffer\n");
    errors++;
    goto end0;
  }

  for (int i=0; i<HYPER_BUFF_SIZE; i++)
  {
    buff[i] = i;
  }

  if (pi_ram_open(&hyper))
  {
    printf("[HYPERRAM] Failed to open ram\n");
    errors++;
    goto end1;
  }

  if (pi_ram_alloc(&hyper, &hyper_buff, HYPER_BUFF_SIZE))
  {
    printf("[HYPERRAM] Failed to alloc ram\n");
    errors++;
    goto end2;
  }

  pi_ram_write(&hyper, hyper_buff, buff, HYPER_BUFF_SIZE);

  memset(buff, 0, HYPER_BUFF_SIZE);

  pi_ram_read(&hyper, hyper_buff, buff, HYPER_BUFF_SIZE);


  for (int i=0; i<HYPER_BUFF_SIZE; i++)
  {
    unsigned char expected = i & 0xff;

    if (expected != buff[i])
    {
      errors++;
    }
  }

  if (errors)
    printf("[HYPERRAM] Didn't get expected buffer\n");

  pi_ram_free(&hyper, hyper_buff, HYPER_BUFF_SIZE);

end2:
  pi_ram_close(&hyper);

end1:
  pi_l2_free(buff, HYPER_BUFF_SIZE);

end0:

  if (errors)
    printf("[HYPERRAM] Failed\n");
  else
    printf("[HYPERRAM] Passed\n");

  return errors;
}
