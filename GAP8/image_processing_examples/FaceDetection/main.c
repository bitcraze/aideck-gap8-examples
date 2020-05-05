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
//#include "ImgIO.h"

/* PMSIS includes */
#include "pmsis.h"
#include "bsp/buffer.h"

/* PMSIS BSP includes */
#if defined(GAPOC)
#include "bsp/gapoc_a.h"
#else
#include "bsp/gapuino.h"
#endif  /* GAPOC */
#if defined(HIMAX)
#include "bsp/camera/himax.h"
#else
#include "bsp/camera/mt9v034.h"
#endif  /* HIMAX */
#if defined(USE_DISPLAY)
#include "bsp/display/ili9341.h"
#endif  /* USE_DISPLAY */
#if defined(USE_STREAMER)
#include "bsp/transport/nina_w10.h"
#include "tools/frame_streamer.h"
#endif

#include "faceDet.h"
#include "FaceDetKernels.h"
#include "ImageDraw.h"
#include "setup.h"

#if defined(HIMAX)
#define CAM_WIDTH    324
#define CAM_HEIGHT   244
#else
#define CAM_WIDTH    320
#define CAM_HEIGHT   240
#endif  /* HIMAX */

#define LCD_WIDTH    320
#define LCD_HEIGHT   240

static unsigned char *imgBuff0;
static struct pi_device ili;
static pi_buffer_t buffer;
static pi_buffer_t buffer_out;
static struct pi_device cam;

L2_MEM unsigned char *ImageOut;
L2_MEM unsigned int *ImageIntegral;
L2_MEM unsigned int *SquaredImageIntegral;
L2_MEM char str_to_lcd[100];

struct pi_device cluster_dev;
struct pi_cluster_task *task;
struct pi_cluster_conf conf;
ArgCluster_T ClusterCall;

#if defined(USE_DISPLAY)
void setCursor(struct pi_device *device,signed short x, signed short y);
void writeFillRect(struct pi_device *device, unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned short color);
void writeText(struct pi_device *device,char* str,int fontsize);
#endif  /* USE_DISPLAY */

static int open_display(struct pi_device *device)
{
#if defined(USE_DISPLAY)
    struct pi_ili9341_conf ili_conf;

    pi_ili9341_conf_init(&ili_conf);

    pi_open_from_conf(device, &ili_conf);

    if (pi_display_open(device))
    {
        return -1;
    }
#endif
    return 0;
}

#if defined(USE_CAMERA)
#if defined(HIMAX)
static int open_camera_himax(struct pi_device *device)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;

  return 0;
}
#else
static int open_camera_mt9v034(struct pi_device *device)
{
  struct pi_mt9v034_conf cam_conf;

  pi_mt9v034_conf_init(&cam_conf);
  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;

  return 0;
}
#endif  /* HIMAX */
#endif  /* USE_CAMERA */


static int open_camera(struct pi_device *device)
{
    #if defined(USE_CAMERA)
    #if defined(HIMAX)
    return open_camera_himax(device);
    #else
    return open_camera_mt9v034(device);
    #endif  /* HIMAX */
    #else
    return 0;
    #endif  /* USE_CAMERA */
}

#if defined(USE_STREAMER)

static pi_task_t task1;
static pi_task_t task2;
static struct pi_device wifi;
static frame_streamer_t *streamer1;
static volatile int stream1_done;
//UART init param
L2_MEM struct pi_uart_conf uart_conf;
L2_MEM struct pi_device uart;
L2_MEM uint8_t rec_digit = -1;

static void streamer_handler(void *arg);


static void cam_handler(void *arg)
{

  stream1_done = 0;


return;
}



static void streamer_handler(void *arg)
{
  *(int *)arg = 1;
  if (stream1_done) // && stream2_done)
  {

  }
}
#endif

#if defined(USE_STREAMER)




static int open_wifi(struct pi_device *device)
{
  struct pi_nina_w10_conf nina_conf;

  pi_nina_w10_conf_init(&nina_conf);

  nina_conf.ssid = "Bitcraze";
  nina_conf.passwd = "TrustIsGoodControlIsBetter";
  nina_conf.ip_addr = "192.168.5.49";
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
}


#endif


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
    unsigned int ImgSize = W*H;

    pi_freq_set(PI_FREQ_DOMAIN_FC,250000000);
pi_gpio_pin_configure(&gpio_device, 2, PI_GPIO_OUTPUT);

  pi_task_push_delayed_us(pi_task_callback(&led_task, led_handle, NULL), 500000);
    imgBuff0 = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH*CAM_HEIGHT)*sizeof(unsigned char));
    if (imgBuff0 == NULL)
    {
        printf("Failed to allocate Memory for Image \n");
        pmsis_exit(-1);
    }

    //This can be moved in init
    ImageOut             = (unsigned char *) pmsis_l2_malloc((Wout*Hout)*sizeof(unsigned char));
    ImageIntegral        = (unsigned int *)  pmsis_l2_malloc((Wout*Hout)*sizeof(unsigned int));
    SquaredImageIntegral = (unsigned int *)  pmsis_l2_malloc((Wout*Hout)*sizeof(unsigned int));

    if (ImageOut == 0)
    {
        printf("Failed to allocate Memory for Image (%d bytes)\n", ImgSize*sizeof(unsigned char));
        pmsis_exit(-2);
    }
    if ((ImageIntegral == 0) || (SquaredImageIntegral == 0))
    {
        printf("Failed to allocate Memory for one or both Integral Images (%d bytes)\n", ImgSize*sizeof(unsigned int));
        pmsis_exit(-3);
  }
    printf("malloc done\n");

#if defined(USE_STREAMER)


#endif
#if defined(USE_DISPLAY)

    if (open_display(&ili))
    {
        printf("Failed to open display\n");
        pmsis_exit(-4);
    }
    printf("display done\n");
#endif
    if (open_camera(&cam))
    {
        printf("Failed to open camera\n");
        pmsis_exit(-5);
    }
        uint8_t set_value=3;
    uint8_t reg_value;

    pi_camera_reg_set(&cam, IMG_ORIENTATION, &set_value);
    pi_camera_reg_get(&cam, IMG_ORIENTATION, &reg_value);
#if defined(USE_STREAMER)

  if (open_wifi(&wifi))
  {
    printf("Failed to open wifi\n");
    return -1;
  }

  streamer1 = open_streamer("cam");
  if (streamer1 == NULL)
    return -1;
//  UART init and configure
  pi_uart_conf_init(&uart_conf);
  uart_conf.enable_tx = 1;
  uart_conf.enable_rx = 0;
  pi_open_from_conf(&uart, &uart_conf);
  printf("[UART] Open\n");
  if (pi_uart_open(&uart))
  {
      printf("[UART] open failed !\n");
      pmsis_exit(-1);
  }

#endif
    
    printf("Camera open success\n");

    #if defined(HIMAX)
    buffer.data = imgBuff0+CAM_WIDTH*2+2;
    buffer.stride = 4;

    // WIth Himax, propertly configure the buffer to skip boarder pixels
    pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, imgBuff0+CAM_WIDTH*2+2);
    pi_buffer_set_stride(&buffer, 4);
    #else
    buffer.data = imgBuff0;
    pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, imgBuff0);
    #endif  /* HIMAX */
    
    #if defined(USE_DISPLAY)||defined(USE_STREAMER)
    buffer_out.data = ImageOut;
    buffer_out.stride = 0;
    pi_buffer_init(&buffer_out, PI_BUFFER_TYPE_L2, ImageOut);
    pi_buffer_set_format(&buffer_out, 64, 44, 1, PI_BUFFER_FORMAT_GRAY);

   // pi_buffer_set_stride(&buffer_out, 0);
    #endif /* USE_DISPLAY */
   

    pi_buffer_set_format(&buffer, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

    ClusterCall.ImageIn              = imgBuff0;
    ClusterCall.Win                  = W;
    ClusterCall.Hin                  = H;
    ClusterCall.Wout                 = Wout;
    ClusterCall.Hout                 = Hout;
    ClusterCall.ImageOut             = ImageOut;
    ClusterCall.ImageIntegral        = ImageIntegral;
    ClusterCall.SquaredImageIntegral = SquaredImageIntegral;

    pi_cluster_conf_init(&conf);
    pi_open_from_conf(&cluster_dev, (void*)&conf);
    pi_cluster_open(&cluster_dev);

    //Set Cluster Frequency to max
    pi_freq_set(PI_FREQ_DOMAIN_CL,175000000);

    task = (struct pi_cluster_task *) pmsis_l2_malloc(sizeof(struct pi_cluster_task));
    memset(task, 0, sizeof(struct pi_cluster_task));
    task->entry = (void *)faceDet_cluster_init;
    task->arg = &ClusterCall;

    pi_cluster_send_task_to_cl(&cluster_dev, task);

    task->entry = (void *)faceDet_cluster_main;
    task->arg = &ClusterCall;

    #if defined(USE_DISPLAY)
    //Setting Screen background to white
    writeFillRect(&ili, 0, 0, 240, 320, 0xFFFF);
    setCursor(&ili, 0, 0);
    writeText(&ili,"      Greenwaves \n       Technologies", 2);
    #endif  /* USE_DISPLAY */
    printf("main loop start\n");

    int nb_frames = 0;
    while (1 && (NB_FRAMES == -1 || nb_frames < NB_FRAMES))
    {
        #if defined(USE_CAMERA)
        #if defined(USE_STREAMER)

          pi_camera_control(&cam, PI_CAMERA_CMD_START, 0);
        pi_camera_capture(&cam, imgBuff0, CAM_WIDTH*CAM_HEIGHT);
        pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);
        frame_streamer_send_async(streamer1, &buffer, pi_task_callback(&task1, streamer_handler, (void *)&stream1_done));

        //frame_streamer_send(streamer1, &buffer);

        #else
        pi_camera_control(&cam, PI_CAMERA_CMD_START, 0);
        pi_camera_capture(&cam, imgBuff0, CAM_WIDTH*CAM_HEIGHT);
        pi_camera_control(&cam, PI_CAMERA_CMD_STOP, 0);

        #endif
        #endif  /* USE_CAMERA */
     /*char * ImageName =  "../../../imgTest0.pgm";
    unsigned int Wi, Hi;
    unsigned int Win = CAM_WIDTH, Hin = CAM_HEIGHT;
    if ((ReadImageFromFile(ImageName, &Wi, &Hi, imgBuff0, Win*Hin*sizeof(unsigned char))==0) || (Wi!=Win) || (Hi!=Hin)) {
        printf("Failed to load image %s or dimension mismatch Expects [%dx%d], Got [%dx%d]\n", ImageName, Win, Hin, Wi, Hi);
        return 1;
    }   */
        pi_cluster_send_task_to_cl(&cluster_dev, task);
        printf("end of face detection, faces detected: %d\n", ClusterCall.num_reponse);
        pi_uart_write(&uart, &ClusterCall.num_reponse, 1);          

        //WriteImageToFile("../../../img_out.ppm", CAM_WIDTH, CAM_HEIGHT, imgBuff0);

        #if defined(USE_DISPLAY)
        pi_display_write(&ili, &buffer_out, 40, 40, 160, 120);
        if (ClusterCall.num_reponse)
        {
            sprintf(str_to_lcd, "Face detected: %d\n", ClusterCall.num_reponse);
            setCursor(&ili, 0, 170);
            writeText(&ili, str_to_lcd, 2);
            //sprintf(str_to_lcd,"1 Image/Sec: \n%d uWatt @ 1.2V   \n%d uWatt @ 1.0V   %c", (int)((float)(1/(50000000.f/ClusterCall.cycles)) * 28000.f),(int)((float)(1/(50000000.f/ClusterCall.cycles)) * 16800.f),'\0');
            //sprintf(out_perf_string,"%d  \n%d  %c", (int)((float)(1/(50000000.f/cycles)) * 28000.f),(int)((float)(1/(50000000.f/cycles)) * 16800.f),'\0');
        }
        #endif  /* USE_DISPLAY */

        nb_frames++;
    }
    printf("Test face detection done.\n");
    pmsis_exit(0);
}

int main(void)
{
   printf("\n\t*** PMSIS FaceDetection Test ***\n\n");
   return pmsis_kickoff((void *) test_facedetection);

}

