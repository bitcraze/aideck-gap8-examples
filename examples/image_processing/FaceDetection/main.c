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
#include "bsp/ai_deck.h"
#include "bsp/camera/himax.h"
#include "bsp/bsp.h"
/* Gaplib includes */

#if defined(USE_STREAMER)
#include "cpx.h"
#include "wifi.h"
#endif /* USE_STREAMER */

// All includes for facedetector application
#include "faceDet.h"
#include "FaceDetKernels.h"
#include "ImageDraw.h"
#include "setup.h"

#define IMG_ORIENTATION 0x0101
#define CAM_WIDTH 324
#define CAM_HEIGHT 244

#define IMAGE_OUT_WIDTH 64
#define IMAGE_OUT_HEIGHT 48

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
    printf("Failed to rotate camera image\n");
    return -1;
  }

  pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);

  return 0;
}

static int open_camera(struct pi_device *device)
{
return open_camera_himax(device);
}



#if defined(USE_STREAMER)
typedef enum
{
  RAW_ENCODING = 0,
  JPEG_ENCODING = 1
} __attribute__((packed)) StreamerMode_t;
static StreamerMode_t streamerMode = RAW_ENCODING;

static int wifiConnected = 0;
static int wifiClientConnected = 0;
static CPXPacket_t rxp;
static CPXPacket_t txp;

typedef struct
{
  uint8_t magic;
  uint16_t width;
  uint16_t height;
  uint8_t depth;
  uint8_t type;
  uint32_t size;
} __attribute__((packed)) img_header_t;

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
#endif


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



void test_facedetection(void)
{


  //  UART init with Crazyflie and configure
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
  printf("[UART] open !\n");

  printf("Entering main controller...\n");

  unsigned int W = CAM_WIDTH, H = CAM_HEIGHT;
  unsigned int Wout = 64, Hout = 48;
  unsigned int ImgSize = W * H;

  // Start LED toggle
  pi_gpio_pin_configure(&gpio_device, 2, PI_GPIO_OUTPUT);
  pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);


  imgBuff0 = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH * CAM_HEIGHT) * sizeof(unsigned char));
  if (imgBuff0 == NULL)
  {
    printf("Failed to allocate Memory for Image \n");
    pmsis_exit(-1);
  }

  // Malloc up image buffers to be used in the cluster
  ImageOut = (unsigned char *)pmsis_l2_malloc((Wout * Hout) * sizeof(unsigned char));
  ImageIntegral = (unsigned int *)pmsis_l2_malloc((Wout * Hout) * sizeof(unsigned int));
  SquaredImageIntegral = (unsigned int *)pmsis_l2_malloc((Wout * Hout) * sizeof(unsigned int));
  if (ImageOut == 0)
  {
    printf("Failed to allocate Memory for Image (%d bytes)\n", ImgSize * sizeof(unsigned char));
    pmsis_exit(-2);
  }
  if ((ImageIntegral == 0) || (SquaredImageIntegral == 0))
  {
    printf("Failed to allocate Memory for one or both Integral Images (%d bytes)\n", ImgSize * sizeof(unsigned int));
    pmsis_exit(-3);
  }
  printf("malloc done\n");

  if (open_camera(&cam))
  {
    printf("Failed to open camera\n");
    pmsis_exit(-5);
  }
  cpxInit();

#if defined(USE_STREAMER)

  cpxEnableFunction(CPX_F_WIFI_CTRL);
  BaseType_t xTask;

  xTask = xTaskCreate(rx_task, "rx_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "RX task did not start !\n");
    pmsis_exit(-1);
  }


  cpxInitRoute(CPX_T_GAP8, CPX_T_HOST, CPX_F_APP, &txp.route);


#endif

  cpxPrintToConsole(LOG_TO_CRTP, "--Face detection example --\n");

  // Setup buffer for images
  buffer.data = imgBuff0 + CAM_WIDTH * 2 + 2;
  buffer.stride = 4;

  pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, imgBuff0);
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

  //Set Cluster Frequency to max
  pi_freq_set(PI_FREQ_DOMAIN_CL, 175000000);

  // Send intializer function to cluster
  task = (struct pi_cluster_task *)pmsis_l2_malloc(sizeof(struct pi_cluster_task));
  memset(task, 0, sizeof(struct pi_cluster_task));
  task->entry = (void *)faceDet_cluster_init;
  task->arg = &ClusterCall;
  pi_cluster_send_task_to_cl(&cluster_dev, task);

  // Assign function for main cluster loop
  task->entry = (void *)faceDet_cluster_main;
  task->arg = &ClusterCall;

  printf("main loop start\n");

  // Start looping through images
  int nb_frames = 0;
  while (1 && (NB_FRAMES == -1 || nb_frames < NB_FRAMES))
  {
    // Capture image
    pi_camera_control(&cam, PI_CAMERA_CMD_START, 0);
    pi_camera_capture(&cam, imgBuff0, CAM_WIDTH * CAM_HEIGHT);
    pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);

    // Send task to the cluster and print response
    pi_cluster_send_task_to_cl(&cluster_dev, task);
    printf("end of face detection, faces detected: %d\n", ClusterCall.num_reponse);
    
#if defined(USE_STREAMER)
   if (wifiClientConnected == 1)
    {
        // First send information about the image
    createImageHeaderPacket(&txp, ImgSize, RAW_ENCODING);
    cpxSendPacketBlocking(&txp);

    // Send image
    sendBufferViaCPX(&txp, imgBuff0, ImgSize);
    }
#endif
    // Send result through the uart to the crazyflie as single characters

    nb_frames++;
  }
  printf("Test face detection done.\n");
  pmsis_exit(0);
}

int main(void)
{
  printf("\n\t*** PMSIS FaceDetection Test ***\n\n");
  return pmsis_kickoff((void *)test_facedetection);
}
