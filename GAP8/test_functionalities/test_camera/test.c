/*
 * Copyright (C) 2019 GreenWaves Technologies
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 *
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

// This example shows how to strean the capture of a camera image using
// small buffers.
// This is using aynchronous transfers to make sure a second buffer is always ready
// when the current is finished, to not lose any data.

#include "pmsis.h"
#include "bsp/bsp.h"
#include "bsp/camera.h"
#include "bsp/camera/himax.h"

#include "ImgIO.h"

#define WIDTH    324
#define HEIGHT   244
#define BUFF_SIZE (WIDTH*HEIGHT)

PI_L2 unsigned char *buff;
static struct pi_device camera;
static volatile int done;


static void handle_transfer_end(void *arg)
{
    done = 1;
}

static int open_camera(struct pi_device *device)
{
    printf("Opening Himax camera\n");
    struct pi_himax_conf cam_conf;
    pi_himax_conf_init(&cam_conf);

    cam_conf.format = PI_CAMERA_QVGA;

    pi_open_from_conf(device, &cam_conf);
    if (pi_camera_open(device))
        return -1;

    return 0;
}


int main()
{
    printf("Entering main controller\n");

    #ifdef ASYNC_CAPTURE
    printf("Testing async camera capture\n");

    #else
    printf("Testing normal camera capture\n");
    #endif

    // Open the Himax camera
    if (open_camera(&camera))
    {
        printf("Failed to open camera\n");
        return -1;
    }

    // Rotate camera orientation
    uint8_t set_value=3;
    uint8_t reg_value;

    pi_camera_reg_set(&camera, IMG_ORIENTATION, &set_value);
    pi_camera_reg_get(&camera, IMG_ORIENTATION, &reg_value);
    printf("img orientation %d\n",reg_value);

    // Reserve buffer space for image
    buff = pmsis_l2_malloc(WIDTH*HEIGHT);
    if (buff == NULL){ return -1;}

    #ifdef ASYNC_CAPTURE
    // Start up async capture task
    pi_task_t task;
    pi_camera_capture_async(&camera, buff, BUFF_SIZE, pi_task_callback(&task, handle_transfer_end, NULL));
    #endif
    
    // Start the camera
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    #ifdef ASYNC_CAPTURE
    while(!done){pi_yield();}
    #else
    pi_camera_capture(&camera, buff, BUFF_SIZE);
    #endif
    
    // Stop the camera and immediately close it
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    pi_camera_close(&camera);
    
    // Write to file
    WriteImageToFile("../../../img_out.ppm", WIDTH, HEIGHT, buff);

}