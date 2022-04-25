#ifndef __IMG_PROC_H__
#define __IMG_PROC_H__


void demosaicking(char *input, char* output, int width, int height, int grayscale);

inline void demosaicking(char *input, char* output, int width, int height, int grayscale)
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
                    output[idx] = 0.33*red + 0.33*green + 0.33*blue;

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

#endif