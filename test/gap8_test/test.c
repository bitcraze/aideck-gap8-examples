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
#include "test.h"
#include "stdio.h"


static PI_L2 uint8_t command;
static PI_L2 uint32_t value;

static pi_task_t led_task;
static int led_val = 0;

static struct pi_device gpio_device;


static void led_handle(void *arg)
{
  pi_gpio_pin_write(&gpio_device, 2, led_val);
  led_val ^= 1;
  pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);
}



static void test_gap8(void)
{
  printf("Entering main controller...\n");

 /* pi_pad_set_function(PI_PAD_14_A2_RF_PACTRL2, PI_PAD_14_A2_GPIO_A2_FUNC1);

  struct pi_gpio_conf gpio_conf;
  pi_gpio_conf_init(&gpio_conf);

  pi_open_from_conf(&gpio_device, &gpio_conf);

  if (pi_gpio_open(&gpio_device))
  {
    printf("[LED] Failed to open device\n");
    pmsis_exit(-1);
  }

  pi_gpio_pin_configure(&gpio_device, 2, PI_GPIO_OUTPUT);

  pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);
*/
  struct pi_uart_conf conf;
  struct pi_device device;

  pi_uart_conf_init(&conf);

  pi_open_from_conf(&device, &conf);

  pi_uart_open(&device);

  value = 0xbc;

  pi_uart_write(&device, &value, 1);



  while(1)
  {
    int errors = 0;

    printf("Waiting command\n");

    pi_uart_read(&device, &command, 1);

    printf("Received command 0x%2.2x\n", command);

    switch (command)
    {
      case 0x01:
        errors += test_hyperram();
        errors += test_hyperflash();
        break;

      case 0x02:
        errors += test_himax();
        break;

      case 0x03:
        errors += test_i2c();
        break;

      default:
        if ((command & 0xc0) == 0x40)
          errors += test_gpio(command);
        break;
    }

    value = errors == 0;

    // Work-around for HW bug, this pad alternate should be set to 0 to have uart working
    pi_pad_set_function(PI_PAD_46_B7_SPIM0_SCK, PI_PAD_46_B7_SPIM0_SCK_FUNC0);


    pi_uart_write(&device, &value, 1);
  }

  pmsis_exit(0);
}

int main(void)
{
    return pmsis_kickoff((void *)test_gap8);
}
