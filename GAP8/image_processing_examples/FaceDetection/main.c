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

/* Gaplib includes */
#include "gaplib/ImgIO.h"

#if defined(USE_STREAMER)
#include "bsp/transport/nina_w10.h"
#include "tools/frame_streamer.h"
#endif /* USE_STREAMER */

// All includes for facedetector application
#include "faceDet.h"
#include "FaceDetKernels.h"
#include "ImageDraw.h"
#include "setup.h"

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

#if defined(USE_CAMERA)
// Open himax camera funciton
static int open_camera_himax(struct pi_device *device)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;
  
  pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);

  return 0;
}
#endif /* USE_CAMERA */

static int open_camera(struct pi_device *device)
{
#if defined(USE_CAMERA)
  return open_camera_himax(device);
#else
  return 0;
#endif /* USE_CAMERA */
}

//UART init param
L2_MEM struct pi_uart_conf uart_conf;
L2_MEM struct pi_device uart;
L2_MEM uint8_t rec_digit = -1;

#if defined(USE_STREAMER)
//  Initialize structs and function for streamer through wifi
static pi_task_t task1;
static struct pi_device wifi;
static frame_streamer_t *streamer1;
static volatile int stream1_done;

static void streamer_handler(void *arg)
{
  *(int *)arg = 1;
  if (stream1_done) 
  {
  }
}

static int open_wifi(struct pi_device *device)
{
  struct pi_nina_w10_conf nina_conf;

  pi_nina_w10_conf_init(&nina_conf);

  nina_conf.ssid = "";
  nina_conf.passwd = "";
  nina_conf.ip_addr = "192.168.0.0";
  nina_conf.port = 5555;
  pi_open_from_conf(device, &nina_conf);
  if (pi_transport_open(device))
    return -1;

  return 0;
}

static frame_streamer_t *open_streamer(char *name)
{
  struct frame_streamer_conf frame_streamer_conf;

  frame_streamer_conf_init(&frame_streamer_conf);

  frame_streamer_conf.transport = &wifi;
  frame_streamer_conf.format = FRAME_STREAMER_FORMAT_JPEG;
  frame_streamer_conf.width = CAM_WIDTH;
  frame_streamer_conf.height = CAM_HEIGHT;
  frame_streamer_conf.depth = 1;
  frame_streamer_conf.name = name;

  return frame_streamer_open(&frame_streamer_conf);
} /* USE_STREAMER */

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

#if defined(USE_CAMERA)
  if (open_camera(&cam))
  {
    printf("Failed to open camera\n");
    pmsis_exit(-5);
  }
  uint8_t set_value = 3;
  uint8_t reg_value;

  pi_camera_reg_set(&cam, IMG_ORIENTATION, &set_value);
  pi_camera_reg_get(&cam, IMG_ORIENTATION, &reg_value);
  printf("Camera set up\n");
#endif

#if defined(USE_STREAMER)

  if (open_wifi(&wifi))
  {
    printf("Failed to open wifi\n");
    return -1;
  }
  printf("WIFI connected\n"); // check this with NINA printout

  streamer1 = open_streamer("cam");
  if (streamer1 == NULL)
    return -1;
  printf("Streamer set up\n");

#endif

  //  UART init with Crazyflie and configure
  pi_uart_conf_init(&uart_conf);
  uart_conf.enable_tx = 1;
  uart_conf.enable_rx = 0;
  pi_open_from_conf(&uart, &uart_conf);
  if (pi_uart_open(&uart))
  {
    printf("[UART] open failed !\n");
    pmsis_exit(-1);
  }
  printf("[UART] Open\n");

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
#if defined(USE_CAMERA)
    // Capture image
    pi_camera_control(&cam, PI_CAMERA_CMD_START, 0);
    pi_camera_capture(&cam, imgBuff0, CAM_WIDTH * CAM_HEIGHT);
    pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);
#else
    // Read in image to check if NN is still working
    // TODO: this breaks after the second read....
    char imageName[64];
	  sprintf(imageName, "../../../imgTest%d.pgm", nb_frames);
    printf("Loading %s ...\n", imageName);
    if (ReadImageFromFile(imageName, CAM_WIDTH, CAM_HEIGHT, 1, imgBuff0, CAM_WIDTH * CAM_HEIGHT * sizeof(char), IMGIO_OUTPUT_CHAR, 0))
    {
      printf("Failed to load image %s\n", imageName);
      return 1;
    }
#endif /* USE_CAMERA */
 
    // Send task to the cluster and print response
    pi_cluster_send_task_to_cl(&cluster_dev, task);
    printf("end of face detection, faces detected: %d\n", ClusterCall.num_reponse);
    //WriteImageToFile("../../../img_out.ppm", IMAGE_OUT_WIDTH, IMAGE_OUT_HEIGHT, 1, ImageOut, GRAY_SCALE_IO);

#if defined(USE_STREAMER)
    // Send image to the streamer to see the result
    frame_streamer_send_async(streamer1, &buffer, pi_task_callback(&task1, streamer_handler, (void *)&stream1_done));
#endif
    // Send result through the uart to the crazyflie as single characters
    pi_uart_write(&uart, &ClusterCall.num_reponse, 1);

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
