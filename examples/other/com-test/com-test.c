
/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8 default test application
 *
 * Copyright (C) 2022 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * main.c - Default test application
 *
 * This application is used for testing in the CI/stab-lab, for more
 * useful things look in the examples folder.
 */
#include "pmsis.h"
#include "stdio.h"
#include "bsp/bsp.h"

#include "cpx.h"

static CPXPacket_t rxp;
static CPXPacket_t txp;

typedef struct {
  uint16_t count;
  uint16_t size;
} SourceTest_t;

static void test_task(void *parameters) {
  cpxEnableFunction(CPX_F_TEST);
  while(1) {
    cpxReceivePacketBlocking(CPX_F_TEST, &rxp);

    switch (rxp.data[0]) {
      case 0:
        // Sink, do nothing
        break;
      case 1:
        // echo
        memcpy(txp.data, rxp.data, rxp.dataLength);
        txp.route.source = rxp.route.destination;
        txp.route.destination = rxp.route.source;
        txp.route.function = rxp.route.function;
        txp.route.version = rxp.route.version;
        txp.dataLength = rxp.dataLength;
        cpxSendPacketBlocking(&txp);
        break;
      case 2:
        // Source
        txp.route.source = rxp.route.destination;
        txp.route.destination = rxp.route.source;
        txp.route.function = rxp.route.function;
        txp.route.version = rxp.route.version;
        SourceTest_t * test = (SourceTest_t*) &rxp.data[1];
        txp.dataLength = test->size;

        for (int i = 0; i < test->count; i++) {
          for (int j = 0; j < test->size; j++) {
            txp.data[j] = j;
          }
          cpxSendPacketBlocking(&txp);
        }
        break;
      default:
         printf("Unknown test command!\n");
    }
  }
}

#define LED_PIN 2
static pi_device_t led_gpio_dev;

void hb_task(void *parameters)
{
  (void)parameters;
  char *taskname = pcTaskGetName(NULL);

  // Initialize the LED pin
  pi_gpio_pin_configure(&led_gpio_dev, LED_PIN, PI_GPIO_OUTPUT);

  const TickType_t xDelay = 100 / portTICK_PERIOD_MS;

  while (1)
  {
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 1);
    vTaskDelay(xDelay);
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 0);
    vTaskDelay(xDelay);
  }
}

void start_test_application(void)
{
  struct pi_uart_conf conf;
  struct pi_device device;
  pi_uart_conf_init(&conf);
  conf.baudrate_bps = 115200;

  pi_open_from_conf(&device, &conf);
  if (pi_uart_open(&device))
  {
    printf("[UART] open failed !\n");
    pmsis_exit(-1);
  }

  printf("\n-- GAP8 test application --\n");

  BaseType_t xTask;

  xTask = xTaskCreate(hb_task, "hb_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    printf("HB task did not start !\n");
    pmsis_exit(-1);
  }

  cpxInit();

  xTask = xTaskCreate(test_task, "test_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    printf("Test task did not start!\n");
    pmsis_exit(-1);
  }

  while (1)
  {
    pi_yield();
  }
}

int main(void)
{
  pi_bsp_init();

  // Increase the FC freq to 250 MHz
  pi_freq_set(PI_FREQ_DOMAIN_FC, 250000000);
  pi_pmu_voltage_set(PI_PMU_DOMAIN_FC, 1200);

  return pmsis_kickoff((void *)start_test_application);
}
