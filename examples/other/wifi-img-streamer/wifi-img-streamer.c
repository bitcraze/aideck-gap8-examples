/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8
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
 * WiFi image streamer example
 */
#include "pmsis.h"

#include <string.h>

#include "bsp/bsp.h"
#include "camera_pipeline.h"
#include "cpx.h"
#include "cpx_stream.h"
#include "gaplib/jpeg_encoder.h"
#include "stdio.h"
#include "stream_config.h"
#include "wifi.h"

static int wifi_client_connected;
static CPXPacket_t wifi_rx_packet;
static CPXPacket_t wifi_tx_packet;

static jpeg_encoder_t jpeg_encoder;
static pi_buffer_t jpeg_header;
static pi_buffer_t jpeg_footer;
static pi_buffer_t jpeg_data;
static uint32_t jpeg_header_size;
static uint32_t jpeg_footer_size;

static void wifi_rx_task(void *parameters)
{
  (void)parameters;
  while (1)
  {
    cpxReceivePacketBlocking(CPX_F_WIFI_CTRL, &wifi_rx_packet);
    WiFiCTRLPacket_t *wifi_ctrl = (WiFiCTRLPacket_t *)wifi_rx_packet.data;

    if (wifi_ctrl->cmd == WIFI_CTRL_STATUS_WIFI_CONNECTED)
    {
      cpxPrintToConsole(LOG_TO_CRTP, "Wifi connected (%u.%u.%u.%u)\n",
                        wifi_ctrl->data[0], wifi_ctrl->data[1],
                        wifi_ctrl->data[2], wifi_ctrl->data[3]);
    }
    else if (wifi_ctrl->cmd == WIFI_CTRL_STATUS_CLIENT_CONNECTED)
    {
      wifi_client_connected = wifi_ctrl->data[0];
      cpxPrintToConsole(LOG_TO_CRTP, "Wifi client connection status: %u\n",
                        wifi_client_connected);
    }
  }
}

#ifdef SETUP_WIFI_AP
static void setup_wifi(void)
{
  static char ssid[] = "WiFi streaming example";
  cpxPrintToConsole(LOG_TO_CRTP, "Setting up WiFi AP\n");

  wifi_tx_packet.route.destination = CPX_T_ESP32;
  wifi_rx_packet.route.source = CPX_T_GAP8;
  wifi_tx_packet.route.function = CPX_F_WIFI_CTRL;
  wifi_tx_packet.route.version = CPX_VERSION;
  WiFiCTRLPacket_t *wifi_ctrl = (WiFiCTRLPacket_t *)wifi_tx_packet.data;

  wifi_ctrl->cmd = WIFI_CTRL_SET_SSID;
  memcpy(wifi_ctrl->data, ssid, sizeof(ssid));
  wifi_tx_packet.dataLength = sizeof(ssid);
  cpxSendPacketBlocking(&wifi_tx_packet);

  wifi_ctrl->cmd = WIFI_CTRL_WIFI_CONNECT;
  wifi_ctrl->data[0] = 0x01;
  wifi_tx_packet.dataLength = 2;
  cpxSendPacketBlocking(&wifi_tx_packet);
}
#endif

static int init_jpeg_encoder(void)
{
  struct jpeg_encoder_conf config;
  jpeg_encoder_conf_init(&config);
  config.width = CAMERA_WIDTH;
  config.height = CAMERA_HEIGHT;
  config.flags = 0;
  if (jpeg_encoder_open(&jpeg_encoder, &config))
  {
    return -1;
  }

  jpeg_header.size = 1024;
  jpeg_header.data = pmsis_l2_malloc(jpeg_header.size);
  jpeg_footer.size = 10;
  jpeg_footer.data = pmsis_l2_malloc(jpeg_footer.size);
  jpeg_data.size = 1024 * 15;
  jpeg_data.data = pmsis_l2_malloc(jpeg_data.size);
  if (jpeg_header.data == NULL || jpeg_footer.data == NULL ||
      jpeg_data.data == NULL)
  {
    return -1;
  }

  jpeg_encoder_header(&jpeg_encoder, &jpeg_header, &jpeg_header_size);
  jpeg_encoder_footer(&jpeg_encoder, &jpeg_footer, &jpeg_footer_size);
  return 0;
}

static uint32_t encode_jpeg(pi_buffer_t *frame, uint32_t *jpeg_size)
{
  uint32_t started_at = xTaskGetTickCount();
  jpeg_encoder_process(&jpeg_encoder, frame, &jpeg_data, jpeg_size);
  return xTaskGetTickCount() - started_at;
}

static void send_jpeg(uint32_t jpeg_size, uint32_t image_size)
{
  cpx_stream_send_header(CAMERA_WIDTH, CAMERA_HEIGHT, image_size,
                         JPEG_ENCODING);
  cpx_stream_send_buffer(jpeg_header.data, jpeg_header_size);
  cpx_stream_send_buffer(jpeg_data.data, jpeg_size);
  cpx_stream_send_buffer(jpeg_footer.data, jpeg_footer_size);
}

static void camera_task(void *parameters)
{
  (void)parameters;
  vTaskDelay(2000);
#ifdef SETUP_WIFI_AP
  setup_wifi();
#endif

  cpxPrintToConsole(LOG_TO_CRTP, "Starting camera task...\n");
  if (camera_pipeline_init())
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to initialize camera pipeline\n");
    return;
  }

#if STREAM_ENCODING_MODE == 1
  if (init_jpeg_encoder())
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to initialize JPEG encoder\n");
    return;
  }
#endif

  cpx_stream_init();
  while (1)
  {
    if (!wifi_client_connected)
    {
      camera_pipeline_disconnect();
      vTaskDelay(10);
      continue;
    }

    uint32_t frame_started_at = xTaskGetTickCount();
    CameraFrame_t frame = camera_pipeline_begin_frame();
    uint32_t image_size = CAMERA_WIDTH * CAMERA_HEIGHT;
    uint32_t encoding_time = 0;

#if STREAM_ENCODING_MODE == 1
    uint32_t jpeg_size = 0;
    encoding_time = encode_jpeg(frame.buffer, &jpeg_size);
    image_size = jpeg_header_size + jpeg_size + jpeg_footer_size;
#endif

    uint32_t transfer_started_at = xTaskGetTickCount();
#if STREAM_ENCODING_MODE == 1
    send_jpeg(jpeg_size, image_size);
#else
    cpx_stream_send_header(CAMERA_WIDTH, CAMERA_HEIGHT, image_size,
                           RAW_ENCODING);
    cpx_stream_send_buffer(frame.data, image_size);
#endif

    uint32_t transfer_time = xTaskGetTickCount() - transfer_started_at;
    uint32_t wait_time = camera_pipeline_end_frame();
    uint32_t frame_time = xTaskGetTickCount() - frame_started_at;

#if OUTPUT_PROFILING_DATA
    cpxPrintToConsole(
      LOG_TO_CRTP,
      "q_ms=%u enc_ms=%u bytes=%u tx_ms=%u frame_ms=%u wait_ms=%u "
      "drops=%u\n",
      camera_pipeline_queue_latency(), encoding_time, image_size, transfer_time,
      frame_time, wait_time, camera_pipeline_dropped_frames());
#endif
  }
}

#define LED_PIN 2
static pi_device_t led_gpio_device;

static void heartbeat_task(void *parameters)
{
  (void)parameters;
  pi_gpio_pin_configure(&led_gpio_device, LED_PIN, PI_GPIO_OUTPUT);
  const TickType_t delay = 500 / portTICK_PERIOD_MS;
  while (1)
  {
    pi_gpio_pin_write(&led_gpio_device, LED_PIN, 1);
    vTaskDelay(delay);
    pi_gpio_pin_write(&led_gpio_device, LED_PIN, 0);
    vTaskDelay(delay);
  }
}

static void start_example(void)
{
  struct pi_uart_conf config;
  struct pi_device uart;
  pi_uart_conf_init(&config);
  config.baudrate_bps = 115200;
  pi_open_from_conf(&uart, &config);
  if (pi_uart_open(&uart))
  {
    printf("[UART] open failed !\n");
    pmsis_exit(-1);
  }

  cpxInit();
  cpxEnableFunction(CPX_F_WIFI_CTRL);
  cpxPrintToConsole(LOG_TO_CRTP, "-- WiFi image streamer example --\n");

  BaseType_t task_status;
  task_status = xTaskCreate(heartbeat_task, "heartbeat_task",
                            configMINIMAL_STACK_SIZE * 2, NULL,
                            tskIDLE_PRIORITY + 1, NULL);
  if (task_status != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Heartbeat task did not start !\n");
    pmsis_exit(-1);
  }

  task_status = xTaskCreate(camera_task, "camera_task",
                            configMINIMAL_STACK_SIZE * 4, NULL,
                            tskIDLE_PRIORITY + 1, NULL);
  if (task_status != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Camera task did not start !\n");
    pmsis_exit(-1);
  }

  task_status = xTaskCreate(wifi_rx_task, "wifi_rx_task",
                            configMINIMAL_STACK_SIZE * 2, NULL,
                            tskIDLE_PRIORITY + 1, NULL);
  if (task_status != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "WiFi RX task did not start !\n");
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
  pi_freq_set(PI_FREQ_DOMAIN_FC, 250000000);
  __pi_pmu_voltage_set(PI_PMU_DOMAIN_FC, 1200);
  return pmsis_kickoff((void *)start_example);
}
