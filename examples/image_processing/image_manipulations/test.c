/*
 * Copyright (C) 2019 GreenWaves Technologies
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 *
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 * Kimberly McGuire, Bitcraze (kimberly@bitcraze.io)
 */

// This example shows how to do simple image manipulations on the
// loaded image on the fabric controller of the gap8.

#include "pmsis.h"
#include "bsp/bsp.h"
#include "bsp/camera.h"
#include "bsp/camera/himax.h"

#include "ImgIO.h"

#define WIDTH    324
#define HEIGHT   244
#define BUFF_SIZE (WIDTH*HEIGHT)

PI_L2 unsigned char *image_in;
PI_L2 unsigned char *image_resize;
PI_L2 unsigned char *image_crop;
PI_L2 unsigned char *image_binary;


void resizeImage(char* old_image, int width, int height, int resize_factor, char* new_image, int * new_width, int* new_height)
{
        *new_width = width/resize_factor;
        *new_height = height/resize_factor;

        for(int y=0;y<height;y=resize_factor+y){
            for(int x=0;x<width;x=resize_factor+x){

                int idx = y*width+x;
                int idx_new = (y/resize_factor)*(*new_width)+(x/resize_factor);
                int sum_intensities = 0;
                int counter=0;
                if(y>resize_factor/2&&y<height-resize_factor/2&&x>resize_factor/2&&x<width-resize_factor/2)
                {
                    for(int n=-resize_factor/2;n<resize_factor/2;n++){
                        for(int m=-resize_factor/2;m<resize_factor/2;m++){
                            int idx_window = (y+n)*width+(x+m);
                            sum_intensities = sum_intensities +  old_image[idx_window];
                            counter++;
                        }
                    }
                    new_image[idx_new] = sum_intensities/counter;

                }else{
                    new_image[idx_new] = old_image[idx];
                }
            }
         }
        *new_width = width/resize_factor;
        *new_height = height/resize_factor;
}

void cropImage(char* old_image, int width, int height, char* new_image, int crop_width, int crop_height, int crop_pos_x, int crop_pos_y)
{
        for(int y=crop_pos_y;y<crop_pos_y+crop_height;y++){
            for(int x=crop_pos_x;x<crop_pos_x+crop_width;x++){
                int idx = y*width+x;
                int idx_new = (y-crop_pos_y)*crop_width+(x-crop_pos_x);
                new_image[idx_new] = old_image[idx];
            }
         }
}

void binaryImage(char* old_image, int width, int height, char* new_image, int threshold)
{
        for(int y=0;y<height;y++){
            for(int x=0;x<width;x++){
                int idx=y*width+x;
                    if (old_image[idx]<threshold ){
                       new_image[idx] =255;
                    }else{
                       new_image[idx] = 0;
                    }
                  //printf("%d ",new_image[idx]);
            }
            // printf("\n ");

         }

}

int main()
{
    printf("Entering main controller\n");

    // initialize and read in original image
    image_in = pmsis_l2_malloc(WIDTH*HEIGHT);
    if (image_in == NULL){  return -1;}
    char * ImageName =  "../../../img.ppm";
    unsigned int Wi, Hi;
    unsigned int Win = WIDTH, Hin = HEIGHT;

    if ((ReadImageFromFile(ImageName, &Wi, &Hi, image_in, Win*Hin*sizeof(unsigned char))==0) || (Wi!=Win) || (Hi!=Hin)) {
        printf("Failed to load image %s or dimension mismatch Expects [%dx%d], Got [%dx%d]\n", ImageName, Win, Hin, Wi, Hi);
        return 1;
    }

    // Reduce the size to 6 times as small
    int resize_factor = 6;
    image_resize = pmsis_l2_malloc(WIDTH*HEIGHT/resize_factor);
    if (image_resize == NULL){ return -1;}
    unsigned int Wi_resize, Hi_resize;

    resizeImage(image_in, Win, Hin, resize_factor, image_resize, &Wi_resize, &Hi_resize); 
    printf("Image made smaller\n");

    // Crop image to 28 by 28
    image_crop = pmsis_l2_malloc(28*28);
    if (image_crop == NULL){ return -1;}
    unsigned int Wi_crop=28, Hi_crop=28;
    unsigned int crop_pos_x = 15, crop_pos_y=1;

    if(Wi_crop>Wi_resize || Hi_crop>Hi_resize){printf("crop should not be bigger than original image!"); return -1;}

    cropImage(image_resize, Wi_resize, Hi_resize, image_crop, Wi_crop, Hi_crop, crop_pos_x, crop_pos_y); 
    printf("Image cropped\n");

    image_binary=pmsis_l2_malloc(Wi_crop*Hi_crop);
    if (image_binary == NULL){ return -1;}

    binaryImage(image_crop, Wi_crop, Hi_crop, image_binary, 25); 
    printf("Image binarized\n");

    // Save result
    WriteImageToFile("../../../img_out.ppm", Wi_crop, Hi_crop, image_binary);

}