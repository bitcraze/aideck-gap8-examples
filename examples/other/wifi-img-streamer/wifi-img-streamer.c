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

#include "bsp/bsp.h"
#include "bsp/camera/himax.h"
#include "bsp/buffer.h"
#include "gaplib/jpeg_encoder.h"
#include "stdio.h"

#include "cpx.h"
#include "wifi.h"

#define IMG_ORIENTATION 0x0101
#define CAM_WIDTH 324
// #define CAM_HEIGHT 244
#define CAM_HEIGHT 324

static pi_task_t task1;
static unsigned char *imgBuff1;
static pi_buffer_t buffer1;
static pi_task_t task2;
static unsigned char *imgBuff2;
static pi_buffer_t buffer2;
static pi_task_t task3;
static unsigned char *imgBuff3;
static pi_buffer_t buffer3;

static struct pi_device camera;

static EventGroupHandle_t evGroup;
#define CAPTURE_DONE_BIT (1 << 0)

// Performance menasuring variables
static uint32_t start = 0;
static uint32_t stmStart = 0;
static uint32_t captureTime1 = 0;
static uint32_t captureTime2 = 0;
static uint32_t captureTime3 = 0;
static uint32_t transferTime = 0;
static uint32_t encodingTime = 0;
#define OUTPUT_PROFILING_DATA

static int open_pi_camera_himax(struct pi_device *device)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;

  // rotate image
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  uint8_t set_value = 3; 
  uint8_t reg_value;
  pi_camera_reg_set(&camera, IMG_ORIENTATION, &set_value);
  pi_time_wait_us(1000000);
  pi_camera_reg_get(&camera, IMG_ORIENTATION, &reg_value);
  if (set_value != reg_value)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to rotate camera image\n");
    return -1;
  }

  // SDK uses 0x0B by default, i.e., vt_reg_div=1 and vt_sys_div=1
  //set_value = 
  //pi_camera_reg_set(&camera, /*HIMAX_OSC_CLK_DIV*/0x3060, &set_value);


  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

  pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);

  return 0;
}

static int wifiConnected = 0;
static int wifiClientConnected = 0;

static CPXPacket_t rxp;
static CPXPacket_t rxp_app;

typedef struct
{
  uint8_t cmd;
  uint64_t timestamp; // usec timestamp from STM32
  int16_t x;  // compressed [mm]
  int16_t y;  // compressed [mm]
  int16_t z;  // compressed [mm]
  uint32_t quat; // compressed, see quatcompress.h
} __attribute__((packed)) StatePacket_t;

static QueueHandle_t stateQueue;

void rx_task(void *parameters)
{
  while (1)
  {
    cpxReceivePacketBlocking(CPX_F_WIFI_CTRL, &rxp);

    WiFiCTRLPacket_t * wifiCtrl = (WiFiCTRLPacket_t*) rxp.data;

    switch (wifiCtrl->cmd)
    {
      case WIFI_CTRL_STATUS_WIFI_CONNECTED:
        cpxPrintToConsole(LOG_TO_CRTP, "Wifi connected (%u.%u.%u.%u)\n",
                          wifiCtrl->data[0], wifiCtrl->data[1],
                          wifiCtrl->data[2], wifiCtrl->data[3]);
        wifiConnected = 1;
        break;
      case WIFI_CTRL_STATUS_CLIENT_CONNECTED:
        cpxPrintToConsole(LOG_TO_CRTP, "Wifi client connection status: %u\n", wifiCtrl->data[0]);
        wifiClientConnected = wifiCtrl->data[0];
        break;
      default:
        break;
    }
  }
}

void rx_task_app(void *parameters)
{
  uint64_t last_timestamp = 0;

  while (1)
  {
    cpxReceivePacketBlocking(CPX_F_APP, &rxp_app);
    // put the state in the thread-safe queue
    xQueueOverwrite(stateQueue, rxp_app.data);

    // Detect a clock synchronization event:
    // The clock on the STM was synchronized (== reset to 0) using a broadcast
    // if the current timestamp is smaller than the previous timestamp, since
    // timestamps can only increment otherwise.
    const StatePacket_t* cf_state = (const StatePacket_t*)rxp_app.data;
    if (last_timestamp == 0 || cf_state->timestamp < last_timestamp) {
      stmStart = xTaskGetTickCount();
    }
    last_timestamp = cf_state->timestamp;
  }
}

static void capture_done_cb(void *arg)
{
  if (arg == &task1) {
    captureTime1 = xTaskGetTickCount() - stmStart;
  }
  if (arg == &task2) {
    captureTime2 = xTaskGetTickCount() - stmStart;
  }
  if (arg == &task3) {
    captureTime3 = xTaskGetTickCount() - stmStart;
    xEventGroupSetBits(evGroup, CAPTURE_DONE_BIT);
  }
}

typedef struct
{
  uint8_t magic;
  uint16_t width;
  uint16_t height;
  uint8_t depth;
  uint8_t type;
  uint32_t size;
  uint32_t timestamp; // GAP8 clock in ms
  int16_t x;     // compressed [mm]
  int16_t y;     // compressed [mm]
  int16_t z;     // compressed [mm]
  uint32_t quat; // compressed, see quatcompress.h
} __attribute__((packed)) img_header_t;

static jpeg_encoder_t jpeg_encoder;

typedef enum
{
  RAW_ENCODING = 0,
  JPEG_ENCODING = 1
} __attribute__((packed)) StreamerMode_t;

pi_buffer_t header;
uint32_t headerSize;
pi_buffer_t footer;
uint32_t footerSize;
pi_buffer_t jpeg_data;
uint32_t jpegSize;

static StreamerMode_t streamerMode = RAW_ENCODING;

static CPXPacket_t txp;

void createImageHeaderPacket(CPXPacket_t * packet, uint32_t imgSize, StreamerMode_t imgType, const StatePacket_t* cf_state, uint32_t captureTime) {
  img_header_t *imgHeader = (img_header_t *) packet->data;
  imgHeader->magic = 0xBC;
  imgHeader->width = CAM_WIDTH;
  imgHeader->height = CAM_HEIGHT;
  imgHeader->depth = 1;
  imgHeader->type = imgType;
  imgHeader->size = imgSize;
  imgHeader->timestamp = captureTime;
  imgHeader->x = cf_state->x;
  imgHeader->y = cf_state->y;
  imgHeader->z = cf_state->z;
  imgHeader->quat = cf_state->quat;

  packet->dataLength = sizeof(img_header_t);
}

void sendBufferViaCPX(CPXPacket_t * packet, uint8_t * buffer, uint32_t bufferSize) {
  uint32_t offset = 0;
  uint32_t size = 0;
  do {
    size = sizeof(packet->data);
    if (offset + size > bufferSize)
    {
      size = bufferSize - offset;
    }
    memcpy(packet->data, &buffer[offset], sizeof(packet->data));
    packet->dataLength = size;
    cpxSendPacketBlocking(packet);
    offset += size;
  } while (size == sizeof(packet->data));
}

#ifdef SETUP_WIFI_AP
void setupWiFi(void) {
  static char ssid[] = "WiFi streaming example";
  cpxPrintToConsole(LOG_TO_CRTP, "Setting up WiFi AP\n");
  // Set up the routing for the WiFi CTRL packets
  txp.route.destination = CPX_T_ESP32;
  rxp.route.source = CPX_T_GAP8;
  txp.route.function = CPX_F_WIFI_CTRL;
  WiFiCTRLPacket_t * wifiCtrl = (WiFiCTRLPacket_t*) txp.data;
  
  wifiCtrl->cmd = WIFI_CTRL_SET_SSID;
  memcpy(wifiCtrl->data, ssid, sizeof(ssid));
  txp.dataLength = sizeof(ssid);
  cpxSendPacketBlocking(&txp);
  
  wifiCtrl->cmd = WIFI_CTRL_WIFI_CONNECT;
  wifiCtrl->data[0] = 0x01;
  txp.dataLength = 2;
  cpxSendPacketBlocking(&txp);
}
#endif

void camera_task(void *parameters)
{
  vTaskDelay(2000);

#ifdef SETUP_WIFI_AP
  setupWiFi();
#endif

  cpxPrintToConsole(LOG_TO_CRTP, "Starting camera task...\n");
  uint32_t resolution = CAM_WIDTH * CAM_HEIGHT;
  uint32_t captureSize = resolution * sizeof(unsigned char);

  imgBuff1 = (unsigned char *)pmsis_l2_malloc(captureSize);
  if (imgBuff1 == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate Memory for Image \n");
    return;
  }

  imgBuff2 = (unsigned char *)pmsis_l2_malloc(captureSize);
  if (imgBuff2 == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate Memory for Image \n");
    return;
  }

  imgBuff3 = (unsigned char *)pmsis_l2_malloc(captureSize);
  if (imgBuff3 == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate Memory for Image \n");
    return;
  }
  if (open_pi_camera_himax(&camera))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to open camera\n");
    return;
  }

  struct jpeg_encoder_conf enc_conf;
  jpeg_encoder_conf_init(&enc_conf);
  enc_conf.width = CAM_WIDTH;
  enc_conf.height = CAM_HEIGHT;
  enc_conf.flags = 0; // Move this to the cluster

  if (jpeg_encoder_open(&jpeg_encoder, &enc_conf))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed initialize JPEG encoder\n");
    return;
  }

  pi_buffer_init(&buffer1, PI_BUFFER_TYPE_L2, imgBuff1);
  pi_buffer_set_format(&buffer1, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

  pi_buffer_init(&buffer2, PI_BUFFER_TYPE_L2, imgBuff2);
  pi_buffer_set_format(&buffer2, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

  pi_buffer_init(&buffer3, PI_BUFFER_TYPE_L2, imgBuff3);
  pi_buffer_set_format(&buffer3, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

  header.size = 1024;
  header.data = pmsis_l2_malloc(1024);

  footer.size = 10;
  footer.data = pmsis_l2_malloc(10);

  // This must fit the full encoded JPEG
  jpeg_data.size = 1024 * 15;
  jpeg_data.data = pmsis_l2_malloc(1024 * 15);

  if (header.data == 0 || footer.data == 0 || jpeg_data.data == 0) {
    cpxPrintToConsole(LOG_TO_CRTP, "Could not allocate memory for JPEG image\n");
    return;
  }

  jpeg_encoder_header(&jpeg_encoder, &header, &headerSize);
  jpeg_encoder_footer(&jpeg_encoder, &footer, &footerSize);

  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

  // We're reusing the same packet, so initialize the route once
  cpxInitRoute(CPX_T_GAP8, CPX_T_WIFI_HOST, CPX_F_APP, &txp.route);

  uint32_t imgSize = 0;

  StatePacket_t cf_state;
  cpxPrintToConsole(LOG_TO_CRTP, "Camera ready!\n");
  while (1)
  {
    if (wifiClientConnected == 1)
    {
      // Note about the frame rate. From the datasheet we have
      // frame rate = vt_pix_clk (MHz) x 1 x 10 6 / ( frame_length_lines x line_length_pck )
      // vt_pix_clk seems to be 3 MHz for us (divisors are all set to 1)
      // frame_length_lines = 0x0216 (himax.c SDK) and line_length_pck = 0x0178 (himax.c SDK)
      // This results in 15 fps (66ms)
      // For full resulution, we have min_line_length_pck=0x0178 and min_frame_length_lines=0x0158
      // This would result in 23 fps


      start = xTaskGetTickCount();
      pi_camera_capture_async(&camera, imgBuff1, resolution, pi_task_callback(&task1, capture_done_cb, &task1));
      pi_camera_capture_async(&camera, imgBuff2, resolution, pi_task_callback(&task2, capture_done_cb, &task2));
      pi_camera_capture_async(&camera, imgBuff3, resolution, pi_task_callback(&task3, capture_done_cb, &task3));

      pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
      xEventGroupWaitBits(evGroup, CAPTURE_DONE_BIT, pdTRUE, pdFALSE, (TickType_t)portMAX_DELAY);
      pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

      // get the current state from the STM
      xQueuePeek(stateQueue, &cf_state, 0);

      if (streamerMode == JPEG_ENCODING)
      {
        //jpeg_encoder_process_async(&jpeg_encoder, &buffer, &jpeg_data, pi_task_callback(&task1, encoding_done_cb, NULL));
        //xEventGroupWaitBits(evGroup, JPEG_ENCODING_DONE_BIT, pdTRUE, pdFALSE, (TickType_t)portMAX_DELAY);
        //jpeg_encoder_process_status(&jpegSize, NULL);
        start = xTaskGetTickCount();
        jpeg_encoder_process(&jpeg_encoder, &buffer1, &jpeg_data, &jpegSize);
        encodingTime = xTaskGetTickCount() - start;

        imgSize = headerSize + jpegSize + footerSize;

        // First send information about the image
        createImageHeaderPacket(&txp, imgSize, JPEG_ENCODING, &cf_state, captureTime1);
        cpxSendPacketBlocking(&txp);

        start = xTaskGetTickCount();
        // First send header
        memcpy(txp.data, header.data, headerSize);
        txp.dataLength = headerSize;
        cpxSendPacketBlocking(&txp);
        
        // Send image data
        sendBufferViaCPX(&txp, (uint8_t*) jpeg_data.data, jpegSize);

        // Send footer
        memcpy(txp.data, footer.data, footerSize);
        txp.dataLength = footerSize;
        cpxSendPacketBlocking(&txp);

        transferTime = xTaskGetTickCount() - start;
      }
      else
      {
        imgSize = captureSize;
        start = xTaskGetTickCount();

        // First send information about the image
        createImageHeaderPacket(&txp, imgSize, RAW_ENCODING, &cf_state, captureTime1);
        cpxSendPacketBlocking(&txp);

        // Send image
        sendBufferViaCPX(&txp, imgBuff1, imgSize);
        transferTime = xTaskGetTickCount() - start;

        // second buffer

        // First send information about the image
        createImageHeaderPacket(&txp, imgSize, RAW_ENCODING, &cf_state, captureTime2);
        cpxSendPacketBlocking(&txp);

        // Send image
        sendBufferViaCPX(&txp, imgBuff2, imgSize);
        transferTime = xTaskGetTickCount() - start;

        // third buffer 

        // First send information about the image
        createImageHeaderPacket(&txp, imgSize, RAW_ENCODING, &cf_state, captureTime3);
        cpxSendPacketBlocking(&txp);

        // Send image
        sendBufferViaCPX(&txp, imgBuff3, imgSize);
        transferTime = xTaskGetTickCount() - start;
      }
#ifdef OUTPUT_PROFILING_DATA
      cpxPrintToConsole(LOG_TO_CRTP, "capture=%dms, encoding=%d ms (%d bytes), transfer=%d ms\n",
                        captureTime3-start, encodingTime, imgSize, transferTime);
#endif
    }
    else
    {
      vTaskDelay(10);
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

  const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

  while (1)
  {
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 1);
    vTaskDelay(xDelay);
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 0);
    vTaskDelay(xDelay);
  }
}

void start_example(void)
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

  cpxInit();
  cpxEnableFunction(CPX_F_WIFI_CTRL);
  cpxEnableFunction(CPX_F_APP);

  cpxPrintToConsole(LOG_TO_CRTP, "-- WiFi image streamer example --\n");

  evGroup = xEventGroupCreate();

  // Queue that stores the latest state received from the STM
  stateQueue = xQueueCreate(1, sizeof(StatePacket_t));

  BaseType_t xTask;

  xTask = xTaskCreate(hb_task, "hb_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "HB task did not start !\n");
    pmsis_exit(-1);
  }

  xTask = xTaskCreate(camera_task, "camera_task", configMINIMAL_STACK_SIZE * 4,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Camera task did not start !\n");
    pmsis_exit(-1);
  }

  xTask = xTaskCreate(rx_task, "rx_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "RX task did not start !\n");
    pmsis_exit(-1);
  }

  xTask = xTaskCreate(rx_task_app, "rx_task_app", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "RX app task did not start !\n");
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

  return pmsis_kickoff((void *)start_example);
}