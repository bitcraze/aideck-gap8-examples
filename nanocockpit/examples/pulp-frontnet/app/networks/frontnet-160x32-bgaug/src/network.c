/*
 * network.c
 * Elia Cereda <elia.cereda@idsia.ch>
 * Alessio Burrello <alessio.burrello@unibo.it>
 * Thorir Mar Ingolfsson <thoriri@iis.ee.ethz.ch>
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

#include "config.h"

#define DEFINE_CONSTANTS
#include "network.h"

#include "directional_allocator.h"
#include "mem.h"
#include "net_utils.h"

#include "layer0_BNReluConvolution.h"
#include "layer1_Pooling.h"
#include "layer2_BNReluConvolution.h"
#include "layer3_BNReluConvolution.h"
#include "layer4_BNReluConvolution.h"
#include "layer5_BNReluConvolution.h"
#include "layer6_BNReluConvolution.h"
#include "layer7_BNReluConvolution.h"
#include "layer8_FullyConnected.h"

#include <pmsis.h>

#include <stdbool.h>
#include <string.h>

// Control verbose network output (default: enabled)
#ifndef NETWORK_VERBOSE
  #define NETWORK_VERBOSE 1
#endif

#define FLASH_WEIGHTS_SIZE (311088)
#define L3_INPUT_SIZE (0)
#define L3_OUTPUT_SIZE (0)

typedef struct network_args_s {
  const void *l2_input;
  void *l2_output;
  void *l2_buffer;
  size_t l2_buffer_size;
  int exec;
  pi_task_t *input_done;
} network_args_t;

static network_args_t network_args;
static struct pi_cluster_task network_task;

static void *L3_weights = NULL;
static void *L3_input = NULL;
static void *L3_output = NULL;
int cycle_network_execution;

static void network_run_cluster(void *network_args);
static void execute_layer_fork(void *layer_args);

void network_init() {
  // Load weights and biases from HyperFlash to HyperRAM
  L3_weights = ram_malloc(FLASH_WEIGHTS_SIZE);
  L3_input = ram_malloc(L3_INPUT_SIZE);
  L3_output = ram_malloc(L3_OUTPUT_SIZE);

#if NETWORK_VERBOSE
  printf("\n");
  printf("L3 weights alloc initial\t@ 0x%08x:\t%s\n", (unsigned int)L3_weights, L3_weights?"Ok":"Failed");
  printf("L3   input alloc initial\t@ 0x%08x:\t%s\n", (unsigned int)L3_input, L3_input?"Ok":"Failed");
  printf("L3  output alloc initial\t@ 0x%08x:\t%s\n", (unsigned int)L3_output, L3_output?"Ok":"Failed");
#endif

  void *w_ptr = L3_weights;
  for (int i = 0; i < 8; i++) {
    size_t size = load_file_to_ram(w_ptr, L3_weights_files[i]);
    L3_weights_size[i] = size;
    w_ptr += size;
  }

  uint32_t flash_weights_size = w_ptr - L3_weights;
  if (flash_weights_size != FLASH_WEIGHTS_SIZE) {
    ASSERTION_FAILURE(
      "Flash weights size mismatch: read %dB but expected %dB\n", flash_weights_size, FLASH_WEIGHTS_SIZE
    );
  }
}

void network_terminate() {
  // Free HyperRAM memory
  ram_free(L3_output, L3_OUTPUT_SIZE);
  ram_free(L3_input, L3_INPUT_SIZE);
  ram_free(L3_weights, FLASH_WEIGHTS_SIZE);
}

void network_run_async(const void *l2_input, void *l2_output, void *l2_buffer, size_t l2_buffer_size, int exec, pi_device_t *cluster, pi_task_t *input_done, pi_task_t *network_done) {
  if (l2_buffer_size != NETWORK_L2_BUFFER_SIZE) {
    ASSERTION_FAILURE(
      "L2 buffer size mismatch: got %dB but expected %dB\n", l2_buffer_size, NETWORK_L2_BUFFER_SIZE
    );
  }

  network_args = (network_args_t){
    .l2_input = l2_input, 
    .l2_output = l2_output,
    .l2_buffer = l2_buffer,
    .l2_buffer_size = l2_buffer_size,
    .exec = exec,
    .input_done = input_done
  };

  pi_cluster_task(&network_task, network_run_cluster, &network_args);
  network_task.stack_size = 3500;
  network_task.slave_stack_size = 3400;

  pi_cluster_send_task_to_cl_async(cluster, &network_task, network_done);
}

static void network_run_cluster(void *network_args) {
  network_args_t *args = (network_args_t *)network_args;

  bool l2_input_managed = (args->l2_input >= args->l2_buffer) && (args->l2_input < args->l2_buffer + NETWORK_L2_BUFFER_SIZE);
  bool l2_input_start   = (args->l2_input == args->l2_buffer);
  bool l2_input_end     = (args->l2_input == (args->l2_buffer - NETWORK_INPUT_SIZE));

  int exec = args->exec;

/*
  - initial buffer allocation L2 and L1
  - variable declaration
*/
/* ---------------------------------- */
/* -------- SECTION 0 BEGIN --------- */
/* ---------------------------------- */
  void *L2_output = NULL;
  void *L2_input = NULL;
  void *L2_weights = NULL;
  void *L3_weights_curr = L3_weights;
  void *bypass_activations = NULL;

  int dir = 1;
  int residual_number = 0;
  int bypass_dimension = 0;
  int perf_cyc = 0;
/* ---------------------------------- */
/* --------- SECTION 0 END ---------- */
/* ---------------------------------- */

/*
  - initial copies from L3 of input
  - copies of weights of first 2 layers
*/
/* ---------------------------------- */
/* -------- SECTION 1 BEGIN --------- */
/* ---------------------------------- */
  directional_allocator_init(args->l2_buffer, args->l2_buffer_size);

/* ---------------------------------- */
/* --------- SECTION 1 END ---------- */
/* ---------------------------------- */
  cycle_network_execution = 0;

/* MAIN SECTION
  - for loop over all the layers of the network
  - double buffering using L3
  - check on layers to be executed from L3
  - residual check at the end of each layer
*/
/* ---------------------------------- */
/* -------- SECTION 2 BEGIN --------- */
/* ---------------------------------- */
  int weight_l_cnt = 0; // count how many layers with weights we have processed to increment the weights_L3 pointer
  for (int i = 0; i < 9; i++) {
/* MEMORY ALLOCATION
  - allocate memory if layer is executed from L3;
  - allocate weights
  - read weights
*/
    if (i == 0) {
      if (!l2_input_managed) {
        // Nothing to do
      } else if (l2_input_start) {
        // Reserve space in the directional allocator for the input buffer
        dmalloc(activations_size[i], dir);
      } else if (l2_input_end) {
        ASSERTION_FAILURE("TODO: supplying L2 input at the end of L2 buffer is not implemented");
      } else {
        ASSERTION_FAILURE("L2 input can be inside L2 buffer only if at the beginning or end");
      }

      L2_input = args->l2_input;
    } else if (L3_input_layers[i] == 1) {
      L2_input = dmalloc(activations_size[i], dir);
    }

    L2_output = dmalloc(activations_out_size[i], !dir);

    if (layer_with_weights[i] == 1) {
      L2_weights = dmalloc(weights_size[i], dir);
    }

    if (allocate_layer[i] == 1) {
      cl_ram_read(L2_weights, L3_weights_curr, weights_size[i]);
    }

#if NETWORK_VERBOSE
    if (i == 0 || branch_change[i-1] == 0) {
      if (L3_input_layers[i] == 1)
        checksumL3("L3 input", L3_input, L2_input, L3_activations_size[i], activations_checksum[i][exec]);
      else
        checksum("L2 input", L2_input, activations_size[i], activations_checksum[i][exec]);

      if (layer_with_weights[i]) {
        if (allocate_layer[i] == 0)
          checksumL3("L3 weights", L3_weights_curr, L2_weights, L3_weights_size[weight_l_cnt], weights_checksum[i]);
        else
          checksum("L2 weights", L2_weights, weights_size[i], weights_checksum[i]);
      }
    } else {
      printf("Switching branch, already checked activation\n");
    }
#endif

    layer_args_t largs = {
      .L3_input = (unsigned int) L3_input,
      .L3_output = (unsigned int) L3_output,
      .L3_after_weights = (unsigned int) L3_weights_curr,
      .L2_input = (unsigned int) L2_input,
      .bypass = (unsigned int) bypass_activations,
      .L2_output = (unsigned int) L2_output,
      .L2_weights = (unsigned int) L2_weights,
      .L1_buffer = 0,
      .ram = (unsigned int) get_ram_ptr(),
      .out_mult = (unsigned int) out_mult_vector[i],
      .out_shift = (unsigned int) out_shift_vector[i],
      .layer_id = i
    };

/*
- Execution of the layers_pointers
*/
    // BEGIN PERFORMANCE MEASUREMENTS
    pi_perf_conf(1<<PI_PERF_CYCLES);
    pi_perf_reset();
    pi_perf_stop();
    pi_perf_start();
    execute_layer_fork((void *)&largs);
    pi_perf_stop();
    perf_cyc = pi_perf_read(PI_PERF_CYCLES);
    cycle_network_execution += perf_cyc;
    // END PERFORMANCE MEASUREMENTS

#if NETWORK_VERBOSE
    printf("Layer %s %d ended\n", Layers_name[i], i);
    if (L3_output_layers[i] == 1) {
      checksumL3("L3 output", L3_output, L2_output, L3_activations_out_size[i], activations_out_checksum[i][exec]);
    } else {
      checksum("L2 output", L2_output, activations_out_size[i], activations_out_checksum[i][exec]);
    }
    printf("\n");
#endif

    // TODO: What error?
    // prevents error from compiler
    asm volatile("": : :"memory");
    unsigned int temp = L3_input;
    L3_input = L3_output;
    asm volatile("": : :"memory");
    L3_output = temp;
    asm volatile("": : :"memory");

    // Free memory
    if (layer_with_weights[i] == 1) {
      dfree(weights_size[i], dir);
    }

    if (i == 0) {
      if (!l2_input_managed) {
        // Nothing to do
      } else if (l2_input_start) {
        // Free space in the directional allocator that was reserved for the input
        dfree(activations_size[i], dir);
      } else if (l2_input_end) {
        ASSERTION_FAILURE("TODO: supplying L2 input at the end of L2 buffer is not implemented");
      } else {
        ASSERTION_FAILURE("L2 input can be inside L2 buffer only if at the beginning or end");
      }

      if (args->input_done != NULL) {
        pi_cl_send_task_to_fc(args->input_done);
      }
    } else {
      dfree(activations_size[i], dir);
    }

    if (branch_input[i] == 1) {
      dfree(bypass_dimension, dir);
    }

    L2_input = L2_output;

    // Residual connections
    if (i < 8) {
      if (branch_input[i+1] == 1) {
        bypass_activations = dmalloc(bypass_dimension, !dir);
        residual_number--;
        cl_ram_read(bypass_activations, layers_pointers[residual_number], bypass_dimension);
        cl_ram_free(layers_pointers[residual_number], bypass_dimension);
      }

      // TODO I feel like this should look ahead instead of back
      if (i > 0 && branch_output[i-1]==1 && L3_input_layers[i]==1) { // TODO don't understand this condition
        L3_input = cl_ram_malloc(1500000);
      }
      if (branch_output[i]==1 && L3_output_layers[i]==1) {
        cl_ram_free(L3_input + activations_out_size[i], 1500000 - activations_out_size[i]);
        layers_pointers[residual_number] = L3_input;
        residual_number++;
        bypass_dimension = activations_out_size[i];
      } else
    if (branch_output[i]==1 || branch_change[i] == 1) {
        layers_pointers[residual_number] = cl_ram_malloc(activations_out_size[i]);
        cl_ram_write(layers_pointers[residual_number], L2_output, activations_out_size[i]);
        residual_number++;
        bypass_dimension = activations_out_size[i];
    }

      if (branch_change[i]==1) {
        dfree(activations_out_size[i], !dir);
        L2_input = dmalloc(activations_size[i + 1], !dir);
        cl_ram_read(L2_input, layers_pointers[residual_number - 2], activations_size[i + 1]);
        cl_ram_free(layers_pointers[residual_number - 2], activations_size[i + 1]);
      }
      if (L3_output_layers[i] == 1)
        dfree(activations_out_size[i], !dir);
    }
    if (layer_with_weights[i])
       L3_weights_curr += L3_weights_size[weight_l_cnt++];
    dir = !dir;
  }

/* ---------------------------------- */
/* --------- SECTION 2 END ---------- */
/* ---------------------------------- */

/* ---------------------------------- */
/* -------- SECTION 3 BEGIN --------- */
/* ---------------------------------- */

  memmove(args->l2_output, L2_output, activations_out_size[8]);

#if NETWORK_VERBOSE
  checksum("Final output", args->l2_output, activations_out_size[8], activations_out_checksum[8][exec]);

  print_perf("Final", cycle_network_execution, 14138880);
#endif

/* ---------------------------------- */
/* --------- SECTION 3 END ---------- */
/* ---------------------------------- */
}

static void execute_layer_fork(void *layer_args) {
  layer_args_t *largs = (layer_args_t *)layer_args;
  
  if (pi_core_id() == 0)
    largs->L1_buffer = pmsis_l1_malloc(36700);

  switch (largs->layer_id) {
    case 0:
      pi_cl_team_fork(NUM_CORES, (void *)layer0_BNReluConvolution, largs);
      break;
    case 1:
      pi_cl_team_fork(NUM_CORES, (void *)layer1_Pooling, largs);
      break;
    case 2:
      pi_cl_team_fork(NUM_CORES, (void *)layer2_BNReluConvolution, largs);
      break;
    case 3:
      pi_cl_team_fork(NUM_CORES, (void *)layer3_BNReluConvolution, largs);
      break;
    case 4:
      pi_cl_team_fork(NUM_CORES, (void *)layer4_BNReluConvolution, largs);
      break;
    case 5:
      pi_cl_team_fork(NUM_CORES, (void *)layer5_BNReluConvolution, largs);
      break;
    case 6:
      pi_cl_team_fork(NUM_CORES, (void *)layer6_BNReluConvolution, largs);
      break;
    case 7:
      pi_cl_team_fork(NUM_CORES, (void *)layer7_BNReluConvolution, largs);
      break;
    case 8:
      pi_cl_team_fork(NUM_CORES, (void *)layer8_FullyConnected, largs);
      break;
  }

  if (pi_core_id() == 0)
    pmsis_l1_malloc_free(largs->L1_buffer, 36700);
}

static float network_out_eps[NETWORK_OUTPUT_COUNT] = {
  0.0000073591854743f, 0.0000133181438287f, 0.0000051428660299f, 0.0000095041359600f
};

void network_dequantize_output(const NETWORK_OUTPUT_TYPE *l2_output, float *l2_output_f32) {
  for (int i = 0; i < NETWORK_OUTPUT_COUNT; i++) {
    l2_output_f32[i] = l2_output[i] * network_out_eps[i];
  }
}
