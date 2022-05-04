/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck examples
 *
 * Copyright (C) 2021 Bitcraze AB
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
 * @file classification.c
 *
 *  
 */

#include "classification.h"
#include "bsp/camera/himax.h"
#include "bsp/transport/nina_w10.h"
#include "classificationKernels.h"
#include "gaplib/ImgIO.h"
#include "pmsis.h"
#include "stdio.h"
#include "bsp/bsp.h"
#include "cpx.h"

#define CAM_WIDTH 324
#define CAM_HEIGHT 244

#define CHANNELS 1
#define IO RGB888_IO
#define CAT_LEN sizeof(uint32_t)

#define __XSTR(__s) __STR(__s)
#define __STR(__s) #__s

static pi_task_t task1;
static pi_task_t task2;
static unsigned char *cameraBuffer;
// static unsigned char *imageDemosaiced;
// static signed char *imageCropped;
static signed short *Output_1;
static struct pi_device camera;
static struct pi_device cluster_dev;
static struct pi_cluster_task *task;
static struct pi_cluster_conf conf;

AT_HYPERFLASH_FS_EXT_ADDR_TYPE __PREFIX(_L3_Flash) = 0;

#define IMG_ORIENTATION 0x0101


static void RunNetwork()
{
  __PREFIX(CNN)
  (cameraBuffer, Output_1);
}

static void cam_handler(void *arg)
{

  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

  /* Run inference */
  pi_cluster_send_task_to_cl(&cluster_dev, task);

  if (Output_1[0] > Output_1[1])
  {
    printf("Packet,     confidence: %hd\n", Output_1[0] - Output_1[1]);
  }
  else
  {
    printf("Background, confidence: %hd\n", Output_1[1] - Output_1[0]);
  }

  pi_camera_capture_async(&camera, cameraBuffer, CAM_WIDTH * CAM_HEIGHT, pi_task_callback(&task1, cam_handler, NULL));
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
}

static int open_camera(struct pi_device *device)
{

  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;

  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  uint8_t set_value = 3;
  uint8_t reg_value;
  pi_camera_reg_set(&camera, IMG_ORIENTATION, &set_value);
  pi_time_wait_us(1000000);
  pi_camera_reg_get(&camera, IMG_ORIENTATION, &reg_value);

  if (set_value != reg_value)
  {
    cpxPrintToConsole(LOG_TO_CRTP,"Failed to rotate camera image\n");
    return -1;
  }
              
  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

  pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);
  return 0;
}

int classification()
{
  // For debugging
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

  cpxPrintToConsole(LOG_TO_CRTP, "*** Classification ***\n");

  cpxPrintToConsole(LOG_TO_CRTP, "Starting to open camera\n");

  if (open_camera(&camera))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to open camera\n");
    return -1;
  }
  cpxPrintToConsole(LOG_TO_CRTP,"Opened Camera\n");

  cameraBuffer = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH * CAM_HEIGHT) * sizeof(unsigned char));
  if (cameraBuffer == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed Allocated memory for camera buffer\n");
    return 1;
  }
  cpxPrintToConsole(LOG_TO_CRTP, "Allocated memory for camera buffer\n");

  Output_1 = (signed short *)pmsis_l2_malloc(2 * sizeof(signed short));
  if (Output_1 == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate memory for output\n");
    pmsis_exit(-1);
  }
  cpxPrintToConsole(LOG_TO_CRTP, "Allocated memory for output\n");

  /* Configure CNN task */
  pi_cluster_conf_init(&conf);
  pi_open_from_conf(&cluster_dev, (void *)&conf);
  pi_cluster_open(&cluster_dev);
  task = pmsis_l2_malloc(sizeof(struct pi_cluster_task));
  if (!task)
  {  
    cpxPrintToConsole(LOG_TO_CRTP, "failed to allocate memory for task\n");
  }
  cpxPrintToConsole(LOG_TO_CRTP,"Allocated memory for task\n");

  memset(task, 0, sizeof(struct pi_cluster_task));
  task->entry = &RunNetwork;
  task->stack_size = STACK_SIZE;             // defined in makefile
  task->slave_stack_size = SLAVE_STACK_SIZE; // "
  task->arg = NULL;

  /* Construct CNN */
  if (__PREFIX(CNN_Construct)())
  {
    cpxPrintToConsole(LOG_TO_CRTP,"Failed to construct CNN\n");
    pmsis_exit(-5);
  }
  cpxPrintToConsole(LOG_TO_CRTP,"Constructed CNN\n");

  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  pi_camera_capture_async(&camera, cameraBuffer, CAM_WIDTH * CAM_HEIGHT, pi_task_callback(&task1, cam_handler, NULL));
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);

  while (1)
  {
    pi_yield();
  }

  /* Destruct CNN */
  __PREFIX(CNN_Destruct)
  ();

  return 0;
}

int main(void)
{
  return pmsis_kickoff((void *)classification);
}
