/*
 * soc.c
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

#include "soc.h"

#include "config.h"
#include "debug.h"

#include <pmsis.h>

#if defined(__PULP_OS__)
#include "pulp.h"
#define pi_pmu_set_voltage(x, y)      ( rt_voltage_force(RT_VOLTAGE_DOMAIN_MAIN, x, NULL) )
#define pi_fll_get_frequency(x)       ( rt_freq_get(x) )
#define pi_fll_set_frequency(x, y, z) ( rt_freq_set(x, y) )
#endif

void soc_init(void) {
    pi_pmu_set_voltage(SOC_VOLTAGE, 1);
    pi_time_wait_us(100000);
    pi_fll_set_frequency(FLL_SOC, SOC_FREQ_FC, 1);
    pi_time_wait_us(100000);
    pi_fll_set_frequency(FLL_CLUSTER, SOC_FREQ_CL, 1);
    pi_time_wait_us(100000);

	VERBOSE_PRINT(
        "SOC configuration:\t\tVDD %.1fV, FC %luMHz, CL %luMHz\n",
        SOC_VOLTAGE/1000.f, pi_freq_get(PI_FREQ_DOMAIN_FC)/1000000, pi_freq_get(PI_FREQ_DOMAIN_CL)/1000000
    );
}
