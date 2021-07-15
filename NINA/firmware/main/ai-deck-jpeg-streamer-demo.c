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

/*
 * Authors:  Esteban Gougeon, GreenWaves Technologies (esteban.gougeon@greenwaves-technologies.com)
 *           Germain Haugou, GreenWaves Technologies (germain.haugou@greenwaves-technologies.com)
 */

/* 
 * Nina firmware for the AI-deck streaming JPEG demo. This demo takes
 * JPEG data sent from the GAP8 and forwards it to a TCP socket. The data
 * sent on the socket is a continous stream of JPEG images, where the JPEG
 * start-of-frame (0xFF 0xD8) and end-of-frame (0xFF 0xD9) is used to
 * pick out images from the stream.
 * 
 * The Frame Streamer on the GAP8 will start sending data once it's booted.
 * This firmware only uses the JPEG data sent by the Frame Streamer and
 * ignores the rest.
 *
 * The GAP8 communication sequence is described below:
 *
 * GAP8 sends 4 32-bit unsigned integers (nina_req_t) where:
 *  * type - Describes the type of package (0x81 is JPEG data)
 *  * size - The size of data that should be requested
 *  * info - Used for signaling end-of-frame of the JPEG
 *
 * When the frame streamer starts it will send the following packages of type 0x81:
 * 1) The JPEG header (306 bytes hardcoded in the GAP8 SDK) (info == 1)
 * 2) The JPEG footer (2 bytes hardcoded in the GAP8 SDK) (info == 1)
 * 3) Continous JPEG data (excluding header and footer) where info == 0 for everything
 *    except the package with the last data of a frame
 *
 * Note that if you reflash/restart the Nina you will have to restart the GAP8, since it
 * only sends the JPEG header/footer on startup!
 *
 * This firmware will save the JPEG header and footer and send this on the socket
 * bofore and after each frame that is received from the GAP8.
 *
 * Tested with GAP8 SEK version 3.4.
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
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#include "wifi.h"
#include "spi.h"

/* GAP8 streamer packet type for JPEG related data */
#define NINA_W10_CMD_SEND_PACKET  0x81

/* WiFi SSID/password and AP/station is set from menuconfig */
#ifdef CONFIG_USE_AS_AP
#define WIFI_MODE AIDECK_WIFI_MODE_SOFTAP
#define CONFIG_EXAMPLE_SSID NULL
#define CONFIG_EXAMPLE_PASSWORD NULL
#else
#define WIFI_MODE AIDECK_WIFI_MODE_STATION
#endif

/* The LED is connected on GPIO */
#define BLINK_GPIO 4

/* The period of the LED blinking */
static uint32_t blink_period_ms = 500;

/* Log tag for printouts */
static const char *TAG = "demo";

/* GAP8 communication struct */
typedef struct
{
  uint32_t type; /* Is 0x81 for JPEG related data */
  uint32_t size; /* Size of data to request */
  uint32_t info; /* Signals end-of-frame or header/footer packet*/
} __attribute__((packed)) nina_req_t;

/* Pointer to data transferred via SPI */
static uint8_t *spi_buffer;

/* Storage for JPEG header (hardcoded to 306 bytes like in GAP8) */
static uint8_t jpeg_header[306];
/* Storage for JPEG footer (hardcoded to 2 bytes like in GAP8) */
static uint8_t jpeg_footer[2];

/* JPEG communication state (see above) */
typedef enum
{
  HEADER, /* Next packet is header */
  FOOTER, /* Next packet is footer */
  DATA /* Next packet is data */
} ImageState_t;

/* JPEG communication state */
static ImageState_t state = HEADER;

/* Send JPEG data to client connected on WiFi */
static void send_imagedata_to_client(uint32_t type, uint32_t size, uint32_t info, uint8_t *buffer) {
  bool new_frame = (info == 1);

  if (wifi_is_socket_connected())
  {

    wifi_send_packet( (const char*) buffer, size);

    if (new_frame) {
      wifi_send_packet( (const char*) &jpeg_footer, sizeof(jpeg_footer) );
    }

    if (new_frame) {
      wifi_send_packet( (const char*) &jpeg_header, sizeof(jpeg_header) );
    }
  }
}

/* Handle data from JPEG stream */
static void handle_jpeg_stream_data(uint32_t type, uint32_t size, uint32_t info, uint8_t *buffer) {
  switch (state) {
    case HEADER:
      ESP_LOGI(TAG, "Setting JPEG header of %i bytes", size);
      memcpy(&jpeg_header, (uint8_t*) buffer, sizeof(jpeg_header));
      state = FOOTER;
      break;
    case FOOTER:
      ESP_LOGI(TAG, "Setting JPEG footer of %i bytes", size);
      memcpy(&jpeg_footer, (uint8_t*) buffer, sizeof(jpeg_footer));
      state = DATA;
      break;
    case DATA:
      // This could be dangerous if the last part of the image is 12 bytes..
      if (size != 12) {
        send_imagedata_to_client(type, size, info, buffer);
      }
      break;
  }
}

/* Handle packet received from the GAP8 */
static void handle_gap8_package(uint8_t *buffer) {
  nina_req_t *req = (nina_req_t *)buffer;

  switch (req->type)
  {
    case NINA_W10_CMD_SEND_PACKET:
      if (req->info <= 1) {

        uint32_t type = req->type;
        uint32_t size = req->size;
        uint32_t info = req->info;
     
        spi_read_data(&buffer, size*8); // Make sure transfer is word aligned

        handle_jpeg_stream_data(type, size, info, buffer);
      }
      break;
  }
}

/* Task for receiving JPEG data from GAP8 and sending to client via WiFi */
static void img_sending_task(void *pvParameters) {
  spi_init();
  
  // Add a delay so it won't start sending images right at 
  //    the same moment it's connecting with Wifi
  vTaskDelay(pdMS_TO_TICKS(1000));

  while (1)
  {
    int32_t datalength = spi_read_data(&spi_buffer, CMD_PACKET_SIZE);
    if (datalength > 0) {
      handle_gap8_package(spi_buffer);
    }     
  }
}

/* Task for handling WiFi state */
static void wifi_task(void *pvParameters) {
  wifi_bind_socket();
  while (1) {
    blink_period_ms = 500;
    wifi_wait_for_socket_connected();
    blink_period_ms = 100;
    wifi_wait_for_disconnect();
    ESP_LOGI(TAG, "Client disconnected");
  }
}

/* Main application and blinking of LED */
void app_main() {
  gpio_pad_select_gpio(BLINK_GPIO);
  gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(BLINK_GPIO, 1);

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init(WIFI_MODE, CONFIG_EXAMPLE_SSID, CONFIG_EXAMPLE_PASSWORD);

  xTaskCreate(img_sending_task, "img_sending_task", 4096, NULL, 5, NULL);
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);

  while(1) {
    gpio_set_level(BLINK_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(blink_period_ms/2));
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(blink_period_ms/2));
  }
}
