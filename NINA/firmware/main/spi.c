/*
 * Copyright (C) 2019 GreenWaves Technologies
 * Copyright (C) 2020 Bitcraze AB
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

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/spi_slave.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#include "spi.h"

#define GPIO_HANDSHAKE 2
#define GPIO_NOTIF 32
#define GPIO_MOSI 19
#define GPIO_MISO 23
#define GPIO_SCLK 18
#define GPIO_CS 5

#define BUFFER_LENGTH 2048

#define SPI_BUFFER_LEN SPI_MAX_DMA_LEN

static uint8_t *communication_buffer;

void my_post_setup_cb(spi_slave_transaction_t *trans) {
  WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (1<<GPIO_HANDSHAKE));
}

void my_post_trans_cb(spi_slave_transaction_t *trans) {
  WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (1<<GPIO_HANDSHAKE));
}

void spi_init()
{
  esp_err_t ret;

  //Configuration for the SPI bus
  spi_bus_config_t buscfg={
    .mosi_io_num=GPIO_MOSI,
    .miso_io_num=GPIO_MISO,
    .sclk_io_num=GPIO_SCLK
  };

  //Configuration for the SPI slave interface
  spi_slave_interface_config_t slvcfg={
    .mode=0,
    .spics_io_num=GPIO_CS,
    .queue_size=3,
    .flags=0,
    .post_setup_cb=my_post_setup_cb,
    .post_trans_cb=my_post_trans_cb
  };
  
  //Configuration for the handshake line
  gpio_config_t io_conf={
    .intr_type=GPIO_INTR_DISABLE,
    .mode=GPIO_MODE_OUTPUT,
    .pin_bit_mask=(1<<GPIO_HANDSHAKE)
  };

  //Configure handshake line as output
  gpio_config(&io_conf);

  //Configuration for the handshake line
  gpio_config_t io_conf2={
    .intr_type=GPIO_INTR_DISABLE,
    .mode=GPIO_MODE_OUTPUT,
    .pin_bit_mask=(1ULL<<GPIO_NOTIF)
  };

  //Configure handshake line as output
  gpio_config(&io_conf2);

  gpio_set_pull_mode(GPIO_SCLK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(GPIO_CS, GPIO_PULLUP_ONLY);

  WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (1<<GPIO_HANDSHAKE));
  WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (1ULL<<GPIO_NOTIF));


  //Initialize SPI slave interface
  ret=spi_slave_initialize(VSPI_HOST, &buscfg, &slvcfg, 1);
  assert(ret==ESP_OK);

  communication_buffer = (uint8_t*)heap_caps_malloc(SPI_BUFFER_LEN, MALLOC_CAP_DMA);
}

int spi_read_data(uint8_t ** buffer, size_t len) {
  spi_slave_transaction_t t;
  memset(&t, 0, sizeof(t));

  t.length = len;
  t.tx_buffer = NULL;
  t.rx_buffer = communication_buffer;
  t.trans_len = 0;

  if (spi_slave_transmit(VSPI_HOST, &t, portMAX_DELAY))
    return -1;

  *buffer = communication_buffer;

  if (t.trans_len == 0)
    return 0;

  return t.trans_len; // This isn't always working
}