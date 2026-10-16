/*
 * layer2_BNReluConvolution.c
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
// func_name                      layer2_BNReluConvolution
// flag_DW                        0
// optional                       BNReluConv
// FLAG_BATCHNORM                 1
// has_bias                       0
// FLAG_RELU                      1
// type                           uint8_t
// conv_overlap1                  1
// conv_overlap2                  1
// padding_top                    1
// padding_bottom                 1
// padding_left                   1
// padding_right                  1
// stride                         2
// g                              1
// nif                            32
// out_shift                      24
// data_type_x                    uint
// data_type_y                    uint
// data_type_activations          int
// data_type_weights              int
// nof                            32
// factor                         1
// double_buffering               1
// x_h                            24
// x_w                            40
// x_data_size_byte               8
// x_tile_size_nif                32
// x_tile_size_h                  17
// x_tile_size_w                  33
// x_tile_size_byte               17952
// x_tile_size_nif_byte           32
// x_stride_w_byte                1280
// x_stride_c_byte                32
// y_h                            12
// y_w                            20
// y_data_size_byte               8
// act_dim_bit                    64
// y_tile_size_nof                32
// y_tile_size_h                  8
// y_tile_size_w                  16
// y_tile_size_byte               4096
// y_stride_w_byte                640
// y_stride_c_byte                32
// y_tile_size_nof_byte           32
// tile_dim_h                     2
// tile_dim_w                     2
// tile_dim_nof                   1
// tile_dim_nif                   1
// tile_n_in_last                 32
// fs1                            3
// fs2                            3
// W_data_size_byte               8
// b_data_size_byte               32
// W_tile_size_nof                32
// b_size_byte                    0
// W_tile_size_nif                32
// W_tile_size_nif_last           32
// W_tile_size_byte               9216
// W_stride_nof_byte              288
// W_stride_hw_byte               32
// W_tile_nif_byte                32
// W_tile_nif_byte_last           32
// l2_off_k                       9216
// l2_off_lambda                  9472
// k_tile_size_byte               256
// lambda_tile_size_byte          256
// k_size_byte                    256
// lambda_size_byte               256
// k_tile_size_byte_transfer      256
// lambda_tile_size_byte_transfer 256
// bias_tile_size_byte            0
// l1_x_offset                    0
// l1_y_offset                    17960
// l1_W_offset                    22064
// l1_k_offset                    31288
// l1_lambda_offset               31552
// W_tile_size_nof_last           32
// W_tile_size_nif_byte_last      32
// y_tile_size_nof_last           32
// y_tile_size_h_last             4
// y_tile_size_w_last             4
// y_length_nof_byte_last         32
// x_tile_size_nif_last           32
// x_tile_size_nif_byte_last      32
// x_tile_size_h_last             9
// x_tile_size_w_last             9


#include "layer2_BNReluConvolution.h"
#include "pulp.h"
#include "pmsis.h"
#include "dory_get_tile.h"
#include "dory_dma.h"
#include "pulp_nn_kernels.h"


void layer2_BNReluConvolution(
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
  DMA_copy_x.stride_2d = 1280;
  DMA_copy_x.stride_1d = 32;
  DMA_copy_x.dir = 1;
  DMA_copy_x.tid = dory_dma_channel;
  
  DMA_copy_W.hwc_to_chw = 0;
  DMA_copy_W.stride_2d = 288;
  DMA_copy_W.stride_1d = 32;
  DMA_copy_W.number_of_2d_copies = 32;
  DMA_copy_W.number_of_1d_copies = 9;
  DMA_copy_W.dir = 1;
  DMA_copy_W.tid = dory_dma_channel;
  
  DMA_copy_y.hwc_to_chw = 0;
  DMA_copy_y.stride_2d = 640;
  DMA_copy_y.stride_1d = 32;
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
  volatile int64_t *k;
  volatile int64_t *lambda;
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
  volatile uint8_t *im2col;
  im2col = l1_buffer + 31816;
  uint16_t out_mult = out_mult_in;
  uint16_t out_shift = out_shift_in;

  ////////////////////////////
  // First tile transfering //
  ////////////////////////////
  pi_cl_team_barrier(0);

  int total_tiles = 4;
  // tile loop nest
  for(iter=0; iter < total_tiles; iter++) {
    // check if last in any dimension
      x_tile_size_nif = (_i_nif_load+1 == 1) ? 32 : 32;
      x_tile_size_h   = (_i_h_load+1 == 2)   ? 9 : 17;
      x_tile_size_w   = (_i_w_load+1 == 2)   ? 9 : 33;
      x_tile_size_byte = x_tile_size_nif*x_tile_size_h*x_tile_size_w*8/8;
      x_length_nif_byte = (_i_nif_load+1 == 1)   ? 32 : 32;
      // additionally overlap by padding for the first tile after a border one
      //this because in the first tile we use less pixels from x_buffer, since we have the ones of padding
      pad_offset_h=0, pad_offset_w=0;
      if(_i_h_load > 0)
        pad_offset_h = 1;
      if(_i_w_load > 0)
        pad_offset_w = 1;
      y_tile_size_h   = (_i_h_load+1 == 2)   ? 4 : 8;
      y_tile_size_w   = (_i_w_load+1 == 2)   ? 4 : 16;
      y_tile_size_nof = (_i_nof_load+1 == 1) ? 32 : 32;
      y_tile_size_byte = y_tile_size_nof*y_tile_size_h*y_tile_size_w*8/8;
      y_length_nof_byte = (_i_nof_load+1 == 1)   ? 32 : 32;
      W_tile_size_nof = (_i_nof_load+1 == 1) ? 32 : 32;
      W_tile_size_nif = (_i_nif_load+1 == 1) ? 32 : 32;
      W_tile_size_byte = W_tile_size_nof*W_tile_size_nif*8*3*3/8;
      W_length_nif_byte = (_i_nif_load+1 == 1) ? 32 : 32;
      // transfer of next input tile in double buffering
      if (_i_nif_load!=_i_nif_exec || _i_w_load!=_i_w_exec || _i_h_load!=_i_h_exec)
      {
        DMA_copy_x.ext = dory_get_tile_3d(l2_x, _i_h_load, _i_w_load, _i_nif_load, 17, 33, 32, 40, 32,  1, 1,0, pad_offset_h, pad_offset_w, 0, 8);
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
        DMA_copy_W.ext = dory_get_tile_3d(l2_W, _i_nof_load, 0, _i_nif_load, 32, 3*3, 32, 3*3, 32, 0,0,0,0,0,0, 8);
        DMA_copy_W.loc = (l1_buffer + 22064);
        DMA_copy_W.number_of_2d_copies = W_tile_size_nof;
        DMA_copy_W.length_1d_copy = W_length_nif_byte;
        dory_dma_memcpy_async(&DMA_copy_W);
        dory_dma_barrier(&DMA_copy_W);

        DMA_copy_k.ext = (uint32_t) l2_W+9216 + 256*_i_nof_load;
        DMA_copy_k.loc = (uint32_t) l1_buffer + 31288;
        DMA_copy_k.length_1d_copy = (uint16_t) W_tile_size_nof * 8;
        dory_dma_memcpy_async(&DMA_copy_k);
        dory_dma_barrier(&DMA_copy_k);

        DMA_copy_lambda.ext = (uint32_t) l2_W+9472 + 256*_i_nof_load;
        DMA_copy_lambda.loc = (uint32_t) l1_buffer + 31552;
        DMA_copy_lambda.length_1d_copy = (uint16_t) W_tile_size_nof * 8;
        dory_dma_memcpy_async(&DMA_copy_lambda);
        dory_dma_barrier(&DMA_copy_lambda);
      }
    // creation of the pointers to input, output, weights, lambda and k
    x = (uint8_t *) (l1_buffer + 0);
    k = (int64_t *) (l1_buffer + 31288);
    lambda = (int64_t *) (l1_buffer + 31552);
    W = (uint8_t *) (l1_buffer + 22064);
    y = (uint8_t *) (l1_buffer + 17960);
    p_r = 0;
    p_l = 0;
    p_t = 0;
    p_b = 0;
    if (_i_h_load == 0)
      p_t = 1;
    if (_i_w_load == 0)
      p_l = 1;
    if (_i_h_load == 2-1)
      p_b = 1;
    if (_i_w_load == 2-1)
      p_r = 1;
    pi_cl_team_barrier(0);
    pulp_nn_conv_u8_u8_i8(
      x, im2col,
      NULL,
      y, W,
      k, lambda,
      out_mult, out_shift,
      x_tile_size_w, x_tile_size_h, x_tile_size_nif,
      y_tile_size_w, y_tile_size_h, y_tile_size_nof,
      3, 3,
      p_t, p_b, p_l, p_r, 2, 2,
      1, 1
      );
    pi_cl_team_barrier(0);
      DMA_copy_y.ext = dory_get_tile_3d(l2_y, _i_h_load, _i_w_load, _i_nof_load, 8, 16, 32, 20, 32, 0, 0, 0, 0, 0, 0, 8);
      DMA_copy_y.loc = (l1_buffer + 17960);
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
      if(_i_w_load==2) 
      {
        _i_w_load = 0;
        _i_h_load += 1;
        if(_i_h_load==2) 
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
