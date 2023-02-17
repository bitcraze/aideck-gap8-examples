/*
 * Copyright 2019 GreenWaves Technologies, SAS
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

#include "stdio.h"


/* PMSIS includes */
#include "pmsis.h"
#include "bsp/buffer.h"

/* PMSIS BSP includes */
#include "bsp/bsp.h"
#include "bsp/buffer.h"
#include "bsp/ai_deck.h"
#include "bsp/camera/himax.h"

/* Gaplib includes */
// #include "gaplib/ImgIO.h"

#if defined(USE_STREAMER)
#include "cpx.h"
#include "wifi.h"
#endif /* USE_STREAMER */

// All includes for facedetector application
#include "faceDet.h"
#include "FaceDetKernels.h"
#include "ImageDraw.h"
#include "setup.h"

#define CAM_WIDTH 324
#define CAM_HEIGHT 244
#define IMG_ORIENTATION 0x0101

#define IMAGE_OUT_WIDTH 64
#define IMAGE_OUT_HEIGHT 48

static EventGroupHandle_t evGroup;
#define CAPTURE_DONE_BIT (1 << 0)

// Performance menasuring variables
static uint32_t start = 0;
static uint32_t captureTime = 0;
static uint32_t transferTime = 0;
static uint32_t encodingTime = 0;
// #define OUTPUT_PROFILING_DATA

static int wifiConnected = 0;
static int wifiClientConnected = 0;

static pi_task_t task1;

static CPXPacket_t rxp;
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

static void capture_done_cb(void *arg)
{
  xEventGroupSetBits(evGroup, CAPTURE_DONE_BIT);
}

typedef struct
{
  uint8_t magic;
  uint16_t width;
  uint16_t height;
  uint8_t depth;
  uint8_t type;
  uint32_t size;
} __attribute__((packed)) img_header_t;


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

void createImageHeaderPacket(CPXPacket_t * packet, uint32_t imgSize, StreamerMode_t imgType) {
  img_header_t *imgHeader = (img_header_t *) packet->data;
  imgHeader->magic = 0xBC;
  imgHeader->width = CAM_WIDTH;
  imgHeader->height = CAM_HEIGHT;
  imgHeader->depth = 1;
  imgHeader->type = imgType;
  imgHeader->size = imgSize;
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

// Intializing buffers for camera images
static unsigned char *imgBuff0;
static struct pi_device ili;
static pi_buffer_t buffer;
static struct pi_device cam;

// Initialize buffers for images handled in the cluster
static pi_buffer_t buffer_out;
L2_MEM unsigned char *ImageOut;
L2_MEM unsigned int *ImageIntegral;
L2_MEM unsigned int *SquaredImageIntegral;
L2_MEM char str_to_lcd[100];

// Intialize structures for clusters
struct pi_device cluster_dev;
struct pi_cluster_task *task;
struct pi_cluster_conf conf;
ArgCluster_T ClusterCall;

// Open himax camera funciton
static int open_camera_himax(struct pi_device *device)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;
  
    // rotate image
  pi_camera_control(device, PI_CAMERA_CMD_START, 0);
  uint8_t set_value=3;
  uint8_t reg_value;
  pi_camera_reg_set(device, IMG_ORIENTATION, &set_value);
  pi_time_wait_us(1000000);
  pi_camera_reg_get(device, IMG_ORIENTATION, &reg_value);
  if (set_value!=reg_value)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to rotate camera image\n");
    return -1;
  }
  pi_camera_control(device, PI_CAMERA_CMD_STOP, 0);
  pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);

  return 0;
}


static int open_camera(struct pi_device *device)
{
  return open_camera_himax(device);
}

//UART init param
L2_MEM struct pi_uart_conf uart_conf;
L2_MEM struct pi_device uart;
L2_MEM uint8_t rec_digit = -1;


// Functions and init for LED toggle
static pi_task_t led_task;
static int led_val = 0;
static struct pi_device gpio_device;
static void led_handle(void *arg)
{
  pi_gpio_pin_write(&gpio_device, 2, led_val);
  led_val ^= 1;
  pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);
}



void facedetection_task(void)
{
    vTaskDelay(2000);
    cpxInitRoute(CPX_T_GAP8, CPX_T_WIFI_HOST, CPX_F_APP, &txp.route);
    cpxPrintToConsole(LOG_TO_CRTP, "Starting face detection task...\n");


#ifdef SETUP_WIFI_AP
  setupWiFi();
#endif
  
  unsigned int W = CAM_WIDTH, H = CAM_HEIGHT;
  unsigned int Wout = 64, Hout = 48;
  unsigned int ImgSize = W * H;

  // Start LED toggle
  pi_gpio_pin_configure(&gpio_device, 2, PI_GPIO_OUTPUT);
  pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);
  imgBuff0 = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH * CAM_HEIGHT) * sizeof(unsigned char));
  if (imgBuff0 == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate Memory for Image \n");
    pmsis_exit(-1);
  }

  // Malloc up image buffers to be used in the cluster
  ImageOut = (unsigned char *)pmsis_l2_malloc((Wout * Hout) * sizeof(unsigned char));
  ImageIntegral = (unsigned int *)pmsis_l2_malloc((Wout * Hout) * sizeof(unsigned int));
  SquaredImageIntegral = (unsigned int *)pmsis_l2_malloc((Wout * Hout) * sizeof(unsigned int));
  if (ImageOut == 0)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate Memory for Image (%d bytes)\n", ImgSize * sizeof(unsigned char));
    pmsis_exit(-2);
  }
  if ((ImageIntegral == 0) || (SquaredImageIntegral == 0))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate Memory for one or both Integral Images (%d bytes)\n", ImgSize * sizeof(unsigned int));
    pmsis_exit(-3);
  }
  // printf("malloc done\n");

  if (open_camera(&cam))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to open camera\n");
    pmsis_exit(-5);
  }

  //  UART init with Crazyflie and configure
  pi_uart_conf_init(&uart_conf);
  uart_conf.enable_tx = 1;
  uart_conf.enable_rx = 0;
  pi_open_from_conf(&uart, &uart_conf);
  if (pi_uart_open(&uart))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "[UART] open failed !\n");
    pmsis_exit(-1);
  }
  cpxPrintToConsole(LOG_TO_CRTP, "[UART] Open\n");

  // Setup buffer for images
  buffer.data = imgBuff0 + CAM_WIDTH * 2 + 2;
  buffer.stride = 4;

  // WIth Himax, propertly configure the buffer to skip boarder pixels
  pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, imgBuff0 + CAM_WIDTH * 2 + 2);
  pi_buffer_set_stride(&buffer, 4);
  pi_buffer_set_format(&buffer, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

  buffer_out.data = ImageOut;
  buffer_out.stride = 0;
  pi_buffer_init(&buffer_out, PI_BUFFER_TYPE_L2, ImageOut);
  pi_buffer_set_format(&buffer_out, IMAGE_OUT_WIDTH, IMAGE_OUT_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);
  pi_buffer_set_stride(&buffer_out, 0);

  // Assign pointers for cluster structure
  ClusterCall.ImageIn = imgBuff0;
  ClusterCall.Win = W;
  ClusterCall.Hin = H;
  ClusterCall.Wout = Wout;
  ClusterCall.Hout = Hout;
  ClusterCall.ImageOut = ImageOut;
  ClusterCall.ImageIntegral = ImageIntegral;
  ClusterCall.SquaredImageIntegral = SquaredImageIntegral;

  pi_cluster_conf_init(&conf);
  pi_open_from_conf(&cluster_dev, (void *)&conf);
  pi_cluster_open(&cluster_dev);

  //Set Cluster Frequency to 75MHz - max (175000000) leads to more camera failures (unknown why)
  pi_freq_set(PI_FREQ_DOMAIN_CL, 75000000);

  // Send intializer function to cluster
  task = (struct pi_cluster_task *)pmsis_l2_malloc(sizeof(struct pi_cluster_task));
  memset(task, 0, sizeof(struct pi_cluster_task));
  task->entry = (void *)faceDet_cluster_init;
  task->arg = &ClusterCall;
  pi_cluster_send_task_to_cl(&cluster_dev, task);

  // Assign function for main cluster loop
  task->entry = (void *)faceDet_cluster_main;
  task->arg = &ClusterCall;

  // printf("main loop start\n");

  // Start looping through images
  int nb_frames = 0;
  EventBits_t evBits;
  pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);

  while (1 && (NB_FRAMES == -1 || nb_frames < NB_FRAMES))
  {
    // Capture image
    pi_camera_capture_async(&cam, imgBuff0, CAM_WIDTH * CAM_HEIGHT, pi_task_callback(&task1, capture_done_cb, NULL));
    pi_camera_control(&cam, PI_CAMERA_CMD_START, 0);
    // it should really not take longer than 500ms to acquire an image, maybe we could even time out earlier
    evBits = xEventGroupWaitBits(evGroup, CAPTURE_DONE_BIT, pdTRUE, pdFALSE, (TickType_t)(500/portTICK_PERIOD_MS));
    pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);
    // if we didn't succeed in capturing the image (which, especially with high cluster frequencies and dark images can happen) we want to retry
    while((evBits & CAPTURE_DONE_BIT) != CAPTURE_DONE_BIT)
    {
      cpxPrintToConsole(LOG_TO_CRTP, "Failed camera acquisition\n");
      pi_camera_control(&cam, PI_CAMERA_CMD_START, 0);
      evBits = xEventGroupWaitBits(evGroup, CAPTURE_DONE_BIT, pdTRUE, pdFALSE, (TickType_t)(500/portTICK_PERIOD_MS));
      pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);
    }
 
    // Send task to the cluster and print response
    pi_cluster_send_task_to_cl(&cluster_dev, task);
    // cpxPrintToConsole(LOG_TO_CRTP, "end of face detection, faces detected: %d\n", ClusterCall.num_reponse);

#if defined(USE_STREAMER)
    if (wifiClientConnected == 1)
    {
      // First send information about the image
      createImageHeaderPacket(&txp, STREAM_W*STREAM_H, RAW_ENCODING);
      cpxSendPacketBlocking(&txp);
      // Send image
      sendBufferViaCPX(&txp, ImageOut, STREAM_W*STREAM_H);
    }
#endif
    // Send result through the uart to the crazyflie as single characters
    pi_uart_write(&uart, &ClusterCall.num_reponse, 1);

    nb_frames++;
  }
  cpxPrintToConsole(LOG_TO_CRTP, "Test face detection done.\n");
  pmsis_exit(0);
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
  cpxInit();
  cpxEnableFunction(CPX_F_WIFI_CTRL);

  cpxPrintToConsole(LOG_TO_CRTP, "-- WiFi image streamer example --\n");

  evGroup = xEventGroupCreate();

  BaseType_t xTask;

  xTask = xTaskCreate(hb_task, "hb_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "HB task did not start !\n");
    pmsis_exit(-1);
  }

  xTask = xTaskCreate(facedetection_task, "facedetection_task", configMINIMAL_STACK_SIZE * 4,
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
