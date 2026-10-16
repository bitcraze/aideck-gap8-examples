/*
 * network.h
 * Elia Cereda <elia.cereda@idsia.ch>
 * Alessio Burrello <alessio.burrello@unibo.it>
 *
 * Copyright (C) 2022-2025 IDSIA, USI-SUPSI
 *               2019-2020 University of Bologna
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

#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <pmsis.h>

#include <stddef.h>

// Expected size of the L2 buffer used for intermediate computation
#define NETWORK_L2_BUFFER_SIZE (352000) // [bytes]

// Properties of the network input tensor
#define NETWORK_INPUT_TYPE uint8_t
#define NETWORK_INPUT_COUNT (15360) // [elements]
#define NETWORK_INPUT_SIZE (NETWORK_INPUT_COUNT * sizeof(NETWORK_INPUT_TYPE)) // [bytes]

// Properties of the network output tensor
#define NETWORK_OUTPUT_TYPE int32_t
#define NETWORK_OUTPUT_COUNT (4) // [elements]
#define NETWORK_OUTPUT_SIZE (NETWORK_OUTPUT_COUNT * sizeof(NETWORK_OUTPUT_TYPE)) // [bytes]

void network_init();
void network_terminate();
void network_run_async(const void *l2_input, void *l2_output, void *l2_buffer, size_t l2_buffer_size, int exec, pi_device_t *cluster, pi_task_t *input_done, pi_task_t *network_done);

void network_dequantize_output(const NETWORK_OUTPUT_TYPE *l2_output, float *l2_output_f32);

#ifdef DEFINE_CONSTANTS
// allocation of buffers with parameters needed by the network execution
static const char * L3_weights_files[] = {
  "layer0_BNReluConvolution_weights.hex", "layer2_BNReluConvolution_weights.hex", "layer3_BNReluConvolution_weights.hex", "layer4_BNReluConvolution_weights.hex", "layer5_BNReluConvolution_weights.hex", "layer6_BNReluConvolution_weights.hex", "layer7_BNReluConvolution_weights.hex", "layer8_FullyConnected_weights.hex"
};
static int L3_weights_size[8];
static int layers_pointers[9];
static char * Layers_name[9] = {"layer0_BNReluConvolution", "layer1_Pooling", "layer2_BNReluConvolution", "layer3_BNReluConvolution", "layer4_BNReluConvolution", "layer5_BNReluConvolution", "layer6_BNReluConvolution", "layer7_BNReluConvolution", "layer8_FullyConnected"};
static int L3_input_layers[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int L3_output_layers[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int allocate_layer[9] = {1, 0, 1, 1, 1, 1, 1, 1, 1};
static int branch_input[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int branch_output[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int branch_change[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int weights_checksum[9] = {129408, 0, 1217408, 1240539, 2517541, 5044551, 9495994, 20668135, 914121};
static int weights_size[9] = {1312, 0, 9728, 9728, 19456, 37888, 75776, 149504, 7696};
static int activations_checksum[9][1] = {
  {  810934  },
  {  141585  },
  {  51732  },
  {  20816  },
  {  19410  },
  {  5783  },
  {  7242  },
  {  1511  },
  {  12249  }
};
static int activations_size[9] = {15360, 122880, 30720, 7680, 7680, 3840, 3840, 1920, 1920};
static int out_mult_vector[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
static int out_shift_vector[9] = {24, 0, 24, 24, 24, 24, 24, 24, 0};
static int activations_out_checksum[9][1] = {
  {  141585  },
  {  51732  },
  {  20816  },
  {  19410  },
  {  5783  },
  {  7242  },
  {  1511  },
  {  12249  },
  {  1322  }
};
static int activations_out_size[9] = {122880, 30720, 7680, 7680, 3840, 3840, 1920, 1920, 16};
static int L3_activations_size[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int L3_activations_out_size[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int layer_with_weights[9] = {1, 0, 1, 1, 1, 1, 1, 1, 1};
#endif

#endif  // __NETWORK_H__
