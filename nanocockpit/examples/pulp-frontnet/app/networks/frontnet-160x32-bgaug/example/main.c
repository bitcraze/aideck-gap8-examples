/*
 * main.c
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
#include "mem.h"
#include "network.h"

#include "pmsis.h"

#define VERBOSE 1


void cluster_init(pi_device_t *cluster) {
    struct pi_cluster_conf cluster_conf;

    pi_cluster_conf_init(&cluster_conf);
    cluster_conf.id=0;
    
    pi_open_from_conf(cluster, &cluster_conf);

    int32_t status = pi_cluster_open(cluster);
    printf("Cluster init:\t%s\n", status ? "Failed" : "OK");

    if (status) {
        pmsis_exit(status);
    }
}

int main () {
  PMU_set_voltage(1200, 0);
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_FC, 240000000);
  pi_time_wait_us(10000);
  pi_freq_set(PI_FREQ_DOMAIN_CL, 175000000);
  pi_time_wait_us(10000);
  
  pi_device_t cluster;
  pi_task_t network_done;

  cluster_init(&cluster);

/*
    Opening of Filesystem and Ram
*/
  mem_init();
  network_init();
    
  size_t l2_buffer_size = NETWORK_L2_BUFFER_SIZE;
  void *l2_buffer = pi_l2_malloc(l2_buffer_size);
  printf("Network:\t\t\t%s, %dB @ L2, 0x%08x\n", l2_buffer?"OK":"Failed", l2_buffer_size, l2_buffer);
  if (!l2_buffer) {
      pmsis_exit(-1);
  }

  /*
    Allocating space for input
  */
  void *l2_input = pi_l2_malloc(NETWORK_INPUT_SIZE);
#ifdef VERBOSE
  printf("\nL2 input alloc initial\t@ 0x%08x:\t%s\n", (unsigned int)l2_input, l2_input?"Ok":"Failed");
#endif
  size_t input_size = 1000000;

  void *ram_input = ram_malloc(input_size);
      load_file_to_ram(ram_input, "inputs.hex");
      ram_read(l2_input, ram_input, NETWORK_INPUT_SIZE);
      network_run_async(l2_input, l2_input, l2_buffer, l2_buffer_size, 0, &cluster, /* input_done */ NULL, pi_task_block(&network_done));
      pi_task_wait_on(&network_done);

  ram_free(ram_input, input_size);
  pi_l2_free(l2_input, NETWORK_INPUT_SIZE);
  network_terminate();
}
