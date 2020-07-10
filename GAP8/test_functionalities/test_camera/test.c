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

#define WIDTH    324
#define HEIGHT   244
#define BUFF_SIZE (WIDTH*HEIGHT)

PI_L2 unsigned char *buff;

PI_L2 unsigned char *buff_output;

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

static void demosaicking(char *input, char* output, int width, int height)
{
    int idx = 0;
    int idxr[8];
    char red, blue, green;

    for (int y = 0; y < height ; y++)
    {
        for (int x = 0; x < width ; x++)
        {
            int idx = y * width + x;

            if (x == 0 || y == 0 || x == width-1 || y == height-1)
            {
                output[idx * 3] = 0;
                output[idx * 3 + 1] = 0;
                output[idx * 3 + 2] = 0;
            }
            else
            {

                idxr[0] = (y - 1) * width + (x - 1);
                idxr[1] = (y)*width + (x - 1);
                idxr[2] = (y + 1) * width + (x - 1);
                idxr[3] = (y + 1) * width + (x);
                idxr[4] = (y + 1) * width + (x + 1);
                idxr[5] = (y)*width + (x + 1);
                idxr[6] = (y - 1) * width + (x + 1);
                idxr[7] = (y - 1) * width + (x);

                int x_shift = 0;
                int y_shift = 0;

                if ((x + x_shift) % 2 == 0 && (y + y_shift) % 2 == 0) //R
                {
                    red = input[idx];
                    blue = (input[idxr[0]] + input[idxr[2]] + input[idxr[4]] + input[idxr[6]]) / 4;
                    green = (input[idxr[1]] + input[idxr[3]] + input[idxr[5]] + input[idxr[7]]) / 4;
                }
                else if ((x + x_shift) % 2 == 1 && (y + y_shift) % 2 == 0) //G2
                {
                    red = (input[idxr[1]] + input[idxr[5]]) / 2;
                    blue = (input[idxr[3]] + input[idxr[7]]) / 2;
                    green = input[idx];
                }
                else if ((x + x_shift) % 2 == 0 && (y + y_shift) % 2 == 1) //G1
                {
                    red = (input[idxr[3]] + input[idxr[7]]) / 2;
                    blue = (input[idxr[1]] + input[idxr[5]]) / 2;
                    green = input[idx];
                }
                else if ((x + x_shift) % 2 == 1 && (y + y_shift) % 2 == 1) //B
                {
                    red = (input[idxr[0]] + input[idxr[2]] + input[idxr[4]] + input[idxr[6]]) / 4;
                    blue = input[idx];
                    green = (input[idxr[1]] + input[idxr[3]] + input[idxr[5]] + input[idxr[7]]) / 4;
                }
                else
                {
                    red = 0;
                    green = 0;
                    blue = 0;
                }

                output[idx * 3] = red;
                output[idx * 3 + 1] = green;
                output[idx * 3 + 2] = blue;
            }
        }
    }
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

    #ifdef COLOR_IMAGE
    buff_output = pmsis_l2_malloc(WIDTH*HEIGHT*3);
    if (buff_output == NULL){ return -1;}
    #endif
    printf("Initialized buffers\n");



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

    #ifdef COLOR_IMAGE
    demosaicking(buff, buff_output,WIDTH,HEIGHT);
    #endif

    // Stop the camera and immediately close it
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    pi_camera_close(&camera);
    
    // Write to file
    #ifdef COLOR_IMAGE
    WriteImageToFile("../../../img_color.ppm", WIDTH, HEIGHT,sizeof(uint32_t), buff_output, RGB888_IO);
    #endif
    WriteImageToFile("../../../img_raw.ppm", WIDTH, HEIGHT,sizeof(uint8_t), buff, GRAY_SCALE_IO );
}