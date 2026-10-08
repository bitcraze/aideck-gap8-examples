/*
 * cluster.c
 * Elia Cereda <elia.cereda@idsia.ch>
 *
 * Copyright (C) 2022-2025 IDSIA, USI-SUPSI
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
 * 
 * This software is based on the following publication:
 *    E. Cereda, A. Giusti, D. Palossi. "NanoCockpit: Performance-optimized 
 *    Application Framework for AI-based Autonomous Nanorobotics"
 * We kindly ask for a citation if you use in academic work.
 */

#include "cluster.h"

#include "debug.h"

void cluster_init(pi_device_t *cluster) {
    struct pi_cluster_conf cluster_conf;

    pi_cluster_conf_init(&cluster_conf);
    cluster_conf.id=0;
    
    pi_open_from_conf(cluster, &cluster_conf);

    int32_t status = pi_cluster_open(cluster);
    VERBOSE_PRINT("Cluster init:\t\t\t%s\n", status ? "Failed" : "OK");

    if (status) {
        pmsis_exit(status);
    }
}
