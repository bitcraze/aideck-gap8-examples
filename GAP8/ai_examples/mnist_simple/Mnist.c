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

#include <stdio.h>
#include "Gap8.h"
#include "MnistKernels.h"
#include "Mnist_trained_coef.def"
#include "ImgIO.h"

#define MERGED_DMA_EVENT 1
#define SINGLE_DMA_EVENT 0
#define NORM_IN 4

#define STACK_SIZE      2048
#define MOUNT           1
#define UNMOUNT         0
#define CID             0

#define IMG_W 324
#define IMG_H 244


//Points in the image
RT_L2_DATA unsigned int pointY= 0;
RT_L2_DATA unsigned int pointX= 0;

L2_MEM signed short int *Out_Layer0;
L2_MEM signed short int *Out_Layer1;
L2_MEM signed short int *Out_Layer2;
L2_MEM rt_perf_t *cluster_perf;

RT_L2_DATA signed short *ImageIn;
RT_L2_DATA signed short *ImageIn_16;
RT_L2_DATA unsigned char  *ImageIn_real;
RT_L2_DATA unsigned char  *ImageOut;


static void RunMnist()

{
    int x=0;
    for(unsigned int j=pointY; j<pointY+28; j++)
        for(unsigned int i=pointX; i<pointX+28; i++)
            ImageIn[x++] = (ImageIn_16[j*IMG_W+i]) << NORM_IN;

    Conv5x5ReLUMaxPool2x2_0((short int*)ImageIn   , L2_W_0, L2_B_0, Out_Layer0, 12, 0);
    Conv5x5ReLUMaxPool2x2_1(            Out_Layer0, L2_W_1, L2_B_1, Out_Layer1, 12, 0);
    LinearLayerReLU_2      (            Out_Layer1, L2_W_2, L2_B_2, Out_Layer2, 14, 10, 0);

    //Get Best Result
    int rec_digit=0;
    int highest=Out_Layer2[0];
    for(int i=1;i<10;i++){
        if(highest<Out_Layer2[i]){
            highest=Out_Layer2[i];
            rec_digit=i;
        }
    }
    printf("Recognized Number: %d\n",rec_digit);
}



int main()

{
    char * ImageName;
    unsigned int Wi, Hi;

    //Input image size
    unsigned int Win = 324, Hin = 244;
    unsigned int W   = 28,  H   = 28;


    printf("Entering main controller\n");

    if (rt_event_alloc(NULL, 8)) return -1;

    //Allocating input and output image buffers in L2 memory
    ImageIn_real  = (unsigned char *)  rt_alloc( RT_ALLOC_L2_CL_DATA, Win*Hin*sizeof(unsigned char));
    ImageIn_16    = (short *)          rt_alloc( RT_ALLOC_L2_CL_DATA, Win*Win*sizeof(unsigned short ));
    ImageIn       = (short *)          rt_alloc( RT_ALLOC_L2_CL_DATA, W*H*sizeof(unsigned short));
    ImageOut      = (unsigned char *)  rt_alloc( RT_ALLOC_L2_CL_DATA, W*H*sizeof(unsigned char));

    if (ImageIn_real==0) {
        printf("Failed to allocate Memory for Image (%d bytes)\n", W*H*sizeof(unsigned char));
        return 1;
    }
    if (ImageIn_16==0) {
        printf("Failed to allocate Memory for Image (%d bytes)\n", W*H*sizeof(unsigned short));
        return 1;
    }
    rt_freq_set(RT_FREQ_DOMAIN_FC, 250000000);

    //TODO Move this to Cluster
    Out_Layer0 = (short int *) rt_alloc(RT_ALLOC_L2_CL_DATA, 12*12*sizeof(short int)*128);
    Out_Layer1 = (short int *) rt_alloc(RT_ALLOC_L2_CL_DATA, 4*4*sizeof(short int)*64);
    Out_Layer2 = ( short int *) rt_alloc(RT_ALLOC_L2_CL_DATA, 10*sizeof( short int));

    if (!(Out_Layer0 && Out_Layer1 && Out_Layer2)) {
        printf("Failed to allocated memory, giving up.\n");
        return 0;
    }

    rt_cluster_mount(MOUNT, CID, 0, NULL);
    rt_freq_set(RT_FREQ_DOMAIN_CL, 175000000);

    // Allocate the memory of L2 for the performance structure
    cluster_perf = rt_alloc(RT_ALLOC_L2_CL_DATA, sizeof(rt_perf_t));
    if (cluster_perf == NULL) return -1;

    // Allocate some stacks for cluster in L1, rt_nb_pe returns how many cores exist.
    void *stacks = rt_alloc(RT_ALLOC_CL_DATA, STACK_SIZE*rt_nb_pe());
    if (stacks == NULL) return -1;

    Mnist_L1_Memory = rt_alloc(RT_ALLOC_CL_DATA, _Mnist_L1_Memory_SIZE);
    if(Mnist_L1_Memory == NULL) {
        printf("Mnist_L1_Memory alloc failed\n");
        return -1;
    }
    int correct=0;
    int wrong=0;
    int skipped=0;
    ImageName =  "../../../img_test4.ppm";

    //Points in the image
    pointY= 110;
    pointX= 140;
    //Reading Image from Hyperflash
    if ((ReadImageFromFile(ImageName, &Wi, &Hi, ImageIn_real, Win*Hin*sizeof(unsigned char))==0) || (Wi!=Win) || (Hi!=Hin)) {
        printf("Failed to load image %s or dimension mismatch Expects [%dx%d], Got [%dx%d]\n", ImageName, Win, Hin, Wi, Hi);
        return 1;
    }

    //Convert Image to 16 bit
    for(unsigned int i=0;i<Win*Hin;i++){
        ImageIn_16[i] = ImageIn_real[i];
    }

    // Execute the function "RunCifar10" on the cluster.
    rt_cluster_call(NULL, CID, (void *) RunMnist, NULL, stacks, STACK_SIZE, STACK_SIZE, rt_nb_pe(), NULL);

    // Close the cluster
    rt_cluster_mount(UNMOUNT, CID, 0, NULL);


/*        int x=0;
        for(unsigned int j=pointY; j<pointY+28; j++){
        for(unsigned int i=pointX[; i<pointX+28; i++){
        ImageOut[x++] = ImageIn_real[j*IMG_W+i];
        }
        }
        WriteImageToFile("../../../output.pgm",W,H,ImageOut);
*/

    /*for(int i=0;i<Win*Hin;i++){
      ImageOut[i] = ImageIn[i] >> NORM_IN ;
      }
      WriteImageToFile("../../../output.pgm",W,H,ImageOut);*/

    //Add Buffer Free
    printf("Test success\n");
    return 0;
}
