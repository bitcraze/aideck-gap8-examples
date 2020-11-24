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


static PI_L2 uint32_t i2c_read_value;
static PI_L2 uint8_t read_address[]={0x00,0x00};

int test_i2c()
{
  int errors = 0;
  struct pi_device device;

  printf("[I2C] Start\n");

  struct pi_i2c_conf i2c_conf;
  pi_i2c_conf_init(&i2c_conf);

  i2c_conf.cs = 0x50 << 1;
  i2c_conf.itf = 1;

  pi_open_from_conf(&device, &i2c_conf);

  if (pi_i2c_open(&device))
  {
    printf("[I2C] Failed to open device\n");
    errors++;
    goto end0;
  }

  pi_i2c_write(&device,(uint8_t *)read_address,2,PI_I2C_XFER_STOP);
  pi_i2c_read(&device, (uint8_t *)&i2c_read_value, 4, PI_I2C_XFER_STOP);


  if (i2c_read_value != 0x43427830)
  {
    printf("[I2C] Failed to read magic number\n");
    errors++;
  }


end0:
  if (errors)
    printf("[I2C] Failed\n");
  else
    printf("[I2C] Passed\n");

  return errors;
}
