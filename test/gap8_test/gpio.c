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


static uint32_t i2c_read_value;

int test_gpio(uint32_t command)
{
  uint32_t mask = command & 0x3F;
  int errors = 0;
  struct pi_device device;

  printf("[GPIO] Start\n");

  struct pi_gpio_conf conf;
  pi_gpio_conf_init(&conf);

  pi_open_from_conf(&device, &conf);

  if (pi_gpio_open(&device))
  {
    printf("[GPIO] Failed to open device\n");
    errors++;
    goto end0;
  }

  printf("[GPIO] Setting mask %lx\n", mask);

  pi_pad_set_function(PI_PAD_8_A4_RF_SPIM1_MISO, PI_PAD_8_A4_GPIO_A0_FUNC1);
  pi_pad_set_function(PI_PAD_15_B1_RF_PACTRL3, PI_PAD_15_B1_GPIO_A3_FUNC1);
  pi_pad_set_function(PI_PAD_9_B3_RF_SPIM1_MOSI, PI_PAD_9_B3_GPIO_A1_FUNC1);
  pi_pad_set_function(PI_PAD_32_A13_TIMER0_CH1, PI_PAD_32_A13_GPIO_A18_FUNC1);
  pi_pad_set_function(PI_PAD_10_A5_RF_SPIM1_CSN, PI_PAD_10_A5_GPIO_A2_FUNC1);
  pi_pad_set_function(PI_PAD_11_B4_RF_SPIM1_SCK, PI_PAD_11_B4_GPIO_A3_FUNC1);

  pi_gpio_pin_configure(&device, 0, PI_GPIO_OUTPUT);
  pi_gpio_pin_configure(&device, 3, PI_GPIO_OUTPUT);
  pi_gpio_pin_configure(&device, 1, PI_GPIO_OUTPUT);
  pi_gpio_pin_configure(&device, 18, PI_GPIO_OUTPUT);
  pi_gpio_pin_configure(&device, 2, PI_GPIO_OUTPUT);
  pi_gpio_pin_configure(&device, 3, PI_GPIO_OUTPUT);

  pi_gpio_pin_write(&device, 0, (mask >> 0) & 1);
  pi_gpio_pin_write(&device, 3, (mask >> 1) & 1);
  pi_gpio_pin_write(&device, 1, (mask >> 2) & 1);
  pi_gpio_pin_write(&device, 18, (mask >> 3) & 1);
  pi_gpio_pin_write(&device, 2, (mask >> 4) & 1);
  pi_gpio_pin_write(&device, 3, (mask >> 5) & 1);


end0:
  if (errors)
    printf("[GPIO] Failed\n");
  else
    printf("[GPIO] Passed\n");

  return errors;
}
