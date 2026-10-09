/*
 * layer8_FullyConnected.c
 * Elia Cereda <elia.cereda@idsia.ch>
 * Alessio Burrello <alessio.burrello@unibo.it>
 * Francesco Conti <f.conti@unibo.it>
 *
 * Copyright (C) 2022-2025 IDSIA, USI-SUPSI
 *               2018-2020 University of Bologna
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
// first_layer                    0
// sdk                            gap_sdk
// number_of_clusters             1
// optional_type                  mixed-sw
// func_name                      layer8_FullyConnected
// flag_DW                        0
// optional                       Gemm
// FLAG_BATCHNORM                 0
// has_bias                       1
// FLAG_RELU                      0
// type                           uint8_t
// conv_overlap1                  0
// conv_overlap2                  0
// padding_top                    0
// padding_bottom                 0
// padding_left                   0
// padding_right                  0
// stride                         1
// g                              1
// nif                            1920
// out_shift                      0
// data_type_x                    uint
// data_type_y                    int
// data_type_activations          int
// data_type_weights              int
// nof                            4
// factor                         1
// double_buffering               1
// x_h                            1
// x_w                            1
// x_data_size_byte               8
// x_tile_size_nif                1920
// x_tile_size_h                  1
// x_tile_size_w                  1
// x_tile_size_byte               1920
// x_tile_size_nif_byte           1920
// x_stride_w_byte                1920
// x_stride_c_byte                1920
// y_h                            1
// y_w                            1
// y_data_size_byte               32
// act_dim_bit                    None
// y_tile_size_nof                4
// y_tile_size_h                  1
// y_tile_size_w                  1
// y_tile_size_byte               16
// y_stride_w_byte                16
// y_stride_c_byte                16
// y_tile_size_nof_byte           16
// tile_dim_h                     1
// tile_dim_w                     1
// tile_dim_nof                   1
// tile_dim_nif                   1
// tile_n_in_last                 1920
// fs1                            1
// fs2                            1
// W_data_size_byte               8
// b_data_size_byte               32
// W_tile_size_nof                4
// b_size_byte                    16
// W_tile_size_nif                1920
// W_tile_size_nif_last           1920
// W_tile_size_byte               7680
// W_stride_nof_byte              1920
// W_stride_hw_byte               1920
// W_tile_nif_byte                1920
// W_tile_nif_byte_last           1920
// l2_off_bias                    7680
// k_tile_size_byte               0
// lambda_tile_size_byte          0
// k_size_byte                    0
// lambda_size_byte               0
// bias_tile_size_byte            16
// l1_x_offset                    0
// l1_y_offset                    1928
// l1_W_offset                    1952
// l1_b_offset                    9656
// W_tile_size_nof_last           4
// W_tile_size_nif_byte_last      1920
// y_tile_size_nof_last           4
// y_tile_size_h_last             1
// y_tile_size_w_last             1
// y_length_nof_byte_last         16
// x_tile_size_nif_last           1920
// x_tile_size_nif_byte_last      1920
// x_tile_size_h_last             1
// x_tile_size_w_last             1


#include "layer8_FullyConnected.h"
#include "pulp.h"
#include "pmsis.h"
#include "dory_get_tile.h"
#include "dory_dma.h"
#include "pulp_nn_kernels.h"


void layer8_FullyConnected(
  void *args
) {
  //////////////////////////////////////////////////////////////////////////
  // arguments assigning: keeping same interface between L2 and L3 memory //
  //////////////////////////////////////////////////////////////////////////
  unsigned int *real_arg = (unsigned int *) args;
  unsigned int l3_x =(unsigned int)  real_arg[0];
  unsigned int l3_y =(unsigned int)  real_arg[1];
  unsigned int l3_W =(unsigned int)  real_arg[2];
  unsigned int l2_x =(unsigned int)  real_arg[3];
  unsigned int l2_x_2 =(unsigned int)  real_arg[4];
  unsigned int l2_y =(unsigned int)  real_arg[5];
  unsigned int l2_W =(unsigned int)  real_arg[6];
  unsigned int l1_buffer =(unsigned int)  real_arg[7];
  unsigned int hyperram =(unsigned int)  real_arg[8];
  unsigned int out_mult_in =(unsigned int)  real_arg[9];
  unsigned int out_shift_in = (unsigned int) real_arg[10];

  /////////////////////
  // DMA declaration //
  /////////////////////
  uint32_t dory_dma_channel = dory_dma_allocate();
  volatile DMA_copy DMA_copy_k, DMA_copy_lambda;
  volatile DMA_copy DMA_copy_W, DMA_copy_x, DMA_copy_y;
  volatile DMA_copy DMA_copy_bias;
  DMA_copy_bias.hwc_to_chw = 0;
  DMA_copy_bias.stride_2d = 0;
  DMA_copy_bias.stride_1d = 0;
  DMA_copy_bias.dir = 1;
  DMA_copy_bias.tid = dory_dma_channel;

  DMA_copy_k.hwc_to_chw = 0;
  DMA_copy_k.stride_2d = 0;
  DMA_copy_k.stride_1d = 0;
  DMA_copy_k.number_of_2d_copies = 1;
  DMA_copy_k.number_of_1d_copies = 1;
  DMA_copy_k.dir = 1;
  DMA_copy_k.tid = dory_dma_channel;

  DMA_copy_lambda.hwc_to_chw = 0;
  DMA_copy_lambda.stride_2d = 0;
  DMA_copy_lambda.stride_1d = 0;
  DMA_copy_lambda.number_of_2d_copies = 1;
  DMA_copy_lambda.number_of_1d_copies = 1;
  DMA_copy_lambda.dir = 1;
  DMA_copy_lambda.tid = dory_dma_channel;
  
  DMA_copy_x.hwc_to_chw = 0;
  DMA_copy_x.stride_2d = 1920;
  DMA_copy_x.stride_1d = 1920;
  DMA_copy_x.dir = 1;
  DMA_copy_x.tid = dory_dma_channel;
  
  DMA_copy_W.hwc_to_chw = 0;
  DMA_copy_W.stride_2d = 1920;
  DMA_copy_W.stride_1d = 1920;
  DMA_copy_W.number_of_2d_copies = 4;
  DMA_copy_W.number_of_1d_copies = 1;
  DMA_copy_W.dir = 1;
  DMA_copy_W.tid = dory_dma_channel;
  
  DMA_copy_y.hwc_to_chw = 0;
  DMA_copy_y.stride_2d = 16;
  DMA_copy_y.stride_1d = 16;
  DMA_copy_y.dir = 0;
  DMA_copy_y.tid = dory_dma_channel;

  volatile int p_r, p_l, p_t, p_b;
  volatile  unsigned short x_tile_size_nif;
  volatile unsigned short  x_tile_size_h;
  volatile unsigned short  x_tile_size_w;
  volatile unsigned short  x_tile_size_byte;
  volatile unsigned short  x_length_nif_byte;
  volatile int pad_offset_h, pad_offset_w;
  volatile unsigned short  W_tile_size_nof;
  volatile unsigned short  W_tile_size_nif;
  volatile unsigned short  W_tile_size_byte;
  volatile unsigned short W_length_nif_byte;
  volatile uint8_t *x, *W, *y, *b;
  volatile int y_tile_size_nof;
  volatile int y_tile_size_h;
  volatile int y_tile_size_w;
  volatile int y_tile_size_byte;
  volatile int y_length_nof_byte;
  // last-tile flags
  int iter;
  // tile loop indeces
  int _i_nof_load=0, _i_nif_load=0, _i_h_load=0, _i_w_load=0;
  int _i_nof_exec=1, _i_nif_exec=1, _i_h_exec=1, _i_w_exec=1;
  int has_bias = 1;
  volatile uint8_t *im2col;
  im2col = l1_buffer + 9672;
  uint16_t out_shift = out_shift_in;

  ////////////////////////////
  // First tile transfering //
  ////////////////////////////
  DMA_copy_bias.ext = (uint32_t) l2_W+7680;
  DMA_copy_bias.loc = (uint32_t) (l1_buffer + 9656);
  DMA_copy_bias.number_of_2d_copies = 1;
  DMA_copy_bias.number_of_1d_copies = 1;
  DMA_copy_bias.length_1d_copy = (uint16_t) 16;
  dory_dma_memcpy_async(&DMA_copy_bias);
  dory_dma_barrier(&DMA_copy_bias);

  pi_cl_team_barrier(0);

  int total_tiles = 1;
  // tile loop nest
  for(iter=0; iter < total_tiles; iter++) {
    // check if last in any dimension
      x_tile_size_nif = (_i_nif_load+1 == 1) ? 1920 : 1920;
      x_tile_size_h   = (_i_h_load+1 == 1)   ? 1 : 1;
      x_tile_size_w   = (_i_w_load+1 == 1)   ? 1 : 1;
      x_tile_size_byte = x_tile_size_nif*x_tile_size_h*x_tile_size_w*8/8;
      x_length_nif_byte = (_i_nif_load+1 == 1)   ? 1920 : 1920;
      // additionally overlap by padding for the first tile after a border one
      //this because in the first tile we use less pixels from x_buffer, since we have the ones of padding
      pad_offset_h=0, pad_offset_w=0;
      if(_i_h_load > 0)
        pad_offset_h = 0;
      if(_i_w_load > 0)
        pad_offset_w = 0;
      y_tile_size_h   = (_i_h_load+1 == 1)   ? 1 : 1;
      y_tile_size_w   = (_i_w_load+1 == 1)   ? 1 : 1;
      y_tile_size_nof = (_i_nof_load+1 == 1) ? 4 : 4;
      y_tile_size_byte = y_tile_size_nof*y_tile_size_h*y_tile_size_w*32/8;
      y_length_nof_byte = (_i_nof_load+1 == 1)   ? 16 : 16;
      W_tile_size_nof = (_i_nof_load+1 == 1) ? 4 : 4;
      W_tile_size_nif = (_i_nif_load+1 == 1) ? 1920 : 1920;
      W_tile_size_byte = W_tile_size_nof*W_tile_size_nif*8*1*1/8;
      W_length_nif_byte = (_i_nif_load+1 == 1) ? 1920 : 1920;
      // transfer of next input tile in double buffering
      if (_i_nif_load!=_i_nif_exec || _i_w_load!=_i_w_exec || _i_h_load!=_i_h_exec)
      {
        DMA_copy_x.ext = dory_get_tile_3d(l2_x, _i_h_load, _i_w_load, _i_nif_load, 1, 1, 1920, 1, 1920,  0, 0,0, pad_offset_h, pad_offset_w, 0, 8);
        DMA_copy_x.loc = (l1_buffer + 0);
        DMA_copy_x.number_of_2d_copies = x_tile_size_h;
        DMA_copy_x.number_of_1d_copies = x_tile_size_w;
        DMA_copy_x.length_1d_copy = x_length_nif_byte;
        dory_dma_memcpy_async(&DMA_copy_x);
        dory_dma_barrier(&DMA_copy_x);
      }
      // transfer of next weight tile if changed input or output channels
      if (_i_nif_load!=_i_nif_exec || _i_nof_load!=_i_nof_exec)
      {
        DMA_copy_W.ext = dory_get_tile_3d(l2_W, _i_nof_load, 0, _i_nif_load, 4, 1*1, 1920, 1*1, 1920, 0,0,0,0,0,0, 8);
        DMA_copy_W.loc = (l1_buffer + 1952);
        DMA_copy_W.number_of_2d_copies = W_tile_size_nof;
        DMA_copy_W.length_1d_copy = W_length_nif_byte;
        dory_dma_memcpy_async(&DMA_copy_W);
        dory_dma_barrier(&DMA_copy_W);
      }
    // creation of the pointers to input, output, weights, lambda and k
    x = (uint8_t *) (l1_buffer + 0);
    b = (uint8_t *) (l1_buffer + 9656 + _i_nof_load*16);
    W = (uint8_t *) (l1_buffer + 1952);
    y = (uint8_t *) (l1_buffer + 1928);
    p_r = 0;
    p_l = 0;
    p_t = 0;
    p_b = 0;
    if (_i_h_load == 0)
      p_t = 0;
    if (_i_w_load == 0)
      p_l = 0;
    if (_i_h_load == 1-1)
      p_b = 0;
    if (_i_w_load == 1-1)
      p_r = 0;
    pi_cl_team_barrier(0);
    asm volatile("": : :"memory");
    pulp_nn_linear_u8_i32_i8(
      x, b, y, W,
      x_tile_size_nif, y_tile_size_nof
      );
    pi_cl_team_barrier(0);
      DMA_copy_y.ext = dory_get_tile_3d(l2_y, _i_h_load, _i_w_load, _i_nof_load, 1, 1, 4, 1, 4, 0, 0, 0, 0, 0, 0, 32);
      DMA_copy_y.loc = (l1_buffer + 1928);
      DMA_copy_y.number_of_2d_copies = y_tile_size_h;
      DMA_copy_y.number_of_1d_copies = y_tile_size_w;
      DMA_copy_y.length_1d_copy = y_length_nof_byte;
      dory_dma_memcpy_async(&DMA_copy_y); 
      dory_dma_barrier(&DMA_copy_y);  
    // update prev iterators

    _i_nof_exec = _i_nof_load;
    _i_nif_exec = _i_nif_load;
    _i_h_exec = _i_h_load;
    _i_w_exec = _i_w_load;
      _i_w_load += 1;
      if(_i_w_load==1) 
      {
        _i_w_load = 0;
        _i_h_load += 1;
        if(_i_h_load==1) 
        {
          _i_h_load = 0;
          _i_nof_load += 1;
        }
      }
    pi_cl_team_barrier(0);
  }

  // wait for final write
  dory_dma_free(&DMA_copy_y);
}
