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

#include "gaplib/ImgIO.h"

#include "../../common/img_proc.h"

#define WIDTH    324
#ifdef QVGA_MODE
#define HEIGHT   244
#else
#define HEIGHT   324
#endif
#define BUFF_SIZE (WIDTH*HEIGHT)

PI_L2 unsigned char *buff;

PI_L2 unsigned char *buff_demosaick;

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

#if defined(QVGA_MODE)
    cam_conf.format = PI_CAMERA_QVGA;
#endif

    pi_open_from_conf(device, &cam_conf);
    if (pi_camera_open(device))
        return -1;

    return 0;
}


int test_camera()
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
        pmsis_exit(-1);
    }


    // Rotate camera orientation
    uint8_t set_value=3;
    uint8_t reg_value;

    pi_camera_reg_set(&camera, IMG_ORIENTATION, &set_value);
    pi_camera_reg_get(&camera, IMG_ORIENTATION, &reg_value);
    printf("img orientation %d\n",reg_value);

    #ifdef QVGA_MODE
    set_value=1;
    pi_camera_reg_set(&camera, QVGA_WIN_EN, &set_value);
    pi_camera_reg_get(&camera, QVGA_WIN_EN, &reg_value);
    printf("qvga window enabled %d\n",reg_value);
    #endif

    // Reserve buffer space for image
    buff = pmsis_l2_malloc(BUFF_SIZE);
    if (buff == NULL){ return -1;}

    #ifdef COLOR_IMAGE
    buff_demosaick = pmsis_l2_malloc(BUFF_SIZE*3);
    #else
    buff_demosaick = pmsis_l2_malloc(BUFF_SIZE);
    #endif
    if (buff_demosaick == NULL){ return -1;}
    printf("Initialized buffers\n");



    #ifdef ASYNC_CAPTURE
    // Start up async capture task
    done = 0;
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


    #ifdef COLOR_IMAGE
    demosaicking(buff, buff_demosaick, WIDTH, HEIGHT, 0);
    #else
    demosaicking(buff, buff_demosaick, WIDTH, HEIGHT, 1);
    #endif

    // Write to file
    #ifdef COLOR_IMAGE
    WriteImageToFile("../../../img_color.ppm", WIDTH, HEIGHT, sizeof(uint32_t), buff_demosaick, RGB888_IO);
    #else
    WriteImageToFile("../../../img_gray.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff_demosaick, GRAY_SCALE_IO);
    #endif

    WriteImageToFile("../../../img_raw.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff, GRAY_SCALE_IO );

    pmsis_exit(0);
}

int main(void)
{
    printf("\n\t*** PMSIS Camera with LCD Example ***\n\n");
    return pmsis_kickoff((void *) test_camera);
}
