#include "img_proc.h"

void demosaicking(char *input, char* output, int width, int height, const int grayscale)
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
                if(grayscale)
                {
                    output[idx] = 0;
                }
                else
                {
                    output[idx * 3] = 0;
                    output[idx * 3 + 1] = 0;
                    output[idx * 3 + 2] = 0;
                }
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

                if(grayscale)
                {
                    output[idx] = (red + green + blue)/3;

                }else
                {
                    output[idx * 3] = red;
                    output[idx * 3 + 1] = green;
                    output[idx * 3 + 2] = blue;
                }
                

            }
        }
    }
}

void cluster_demosaicking(void* args)
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t core_id = pi_core_id(), cluster_id = pi_cluster_id();
    plp_example_kernel_instance_i32 *a = (plp_example_kernel_instance_i32*)args;
    char *input = a->srcBuffer;
    char *output = a->resBuffer;
    uint32_t width = a->width;
    uint32_t height = a->height;
    uint32_t nPE = a->nPE;
    uint32_t grayscale = a->grayscale;

    int idxr[8];
    char red, blue, green;

    // uint32_t total = width*height;

    // amount of rows per core, rounded up
    uint32_t y_per_core = (height+nPE-1)/nPE;
    // compute the last element of the area each core has to process
    uint32_t upper_bound = (core_id+1)*y_per_core;
    // as we always rounded up before (to distribute the load as equal as possible) 
    // we need to check if the upper bound is still in our matrix
    if(upper_bound > height ) upper_bound = height; 
    // loop over the area assigned to the core
    for (y = core_id*y_per_core; y < upper_bound; y++) {

        for (int x = 0; x < width ; x++)
        {
            int idx = y * width + x;

            if (x == 0 || y == 0 || x == width-1 || y == height-1)
            {
                if(grayscale)
                {
                    output[idx] = 0;
                }
                else
                {
                    output[idx * 3] = 0;
                    output[idx * 3 + 1] = 0;
                    output[idx * 3 + 2] = 0;
                }
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

                if(grayscale)
                {
                    output[idx] = (red + green + blue)/3;

                }else
                {
                    output[idx * 3] = red;
                    output[idx * 3 + 1] = green;
                    output[idx * 3 + 2] = blue;
                }
                

            }
        }
    }
}

void inverting(char *input, char* output, int width, int height)
{
    int idx = 0;

    for (int y = 0; y < height ; y++)
    {
        for (int x = 0; x < width ; x++)
        {
            int idx = y * width + x;

            output[idx] = 255 - input[idx];
                
        }
    }
}


void cluster_inverting(void* args)
{
    uint32_t idx = 0;
    uint32_t core_id = pi_core_id(), cluster_id = pi_cluster_id();
    plp_example_kernel_instance_i32 *a = (plp_example_kernel_instance_i32*)args;
    char *srcBuffer = a->srcBuffer;
    char *resBuffer = a->resBuffer;
    uint32_t width = a->width;
    uint32_t height = a->height;
    uint32_t nPE = a->nPE;

    uint32_t total = width*height;

    // amount of elements per core, rounded up
    uint32_t per_core = (total+nPE-1)/nPE;
    // compute the last element of the area each core has to process
    uint32_t upper_bound = (core_id+1)*per_core;
    // as we always rounded up before (to distribute the load as equal as possible) 
    // we need to check if the upper bound is still in our matrix
    if(upper_bound > total ) upper_bound = total; 
    // loop over the area assigned to the core
    for (idx = core_id*per_core; idx < upper_bound; idx++) {

            resBuffer[idx] = 255 - srcBuffer[idx];

    }
}