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

  // set configurations in uart
  struct pi_uart_conf conf;
  struct pi_device device;
  pi_uart_conf_init(&conf);
  conf.baudrate_bps =115200;
  //conf.enable_tx = 1;
  //conf.enable_rx = 0;

  // configure LED
  pi_gpio_pin_configure(&gpio_device, 2, PI_GPIO_OUTPUT);

  // Open uart
  pi_open_from_conf(&device, &conf);
  printf("[UART] Open\n");
  if (pi_uart_open(&device))
  {
    printf("[UART] open failed !\n");
    pmsis_exit(-1);
  }

  pi_uart_open(&device);
  value = 0xff;

  while(1)
  {
    // toggle LED when sending information
    pi_gpio_pin_write(&gpio_device, 2, led_val);
    led_val ^= 1;
    pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);     pi_uart_write(&device, &value, 1);
  
  // Write the value to uart
    pi_uart_write(&device, &value, 1);

  }

  pmsis_exit(0);
}

int main(void)
{
    return pmsis_kickoff((void *)test_gap8);
}
