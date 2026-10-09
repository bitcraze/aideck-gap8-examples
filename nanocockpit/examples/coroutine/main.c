/*
 * main.c
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

#include "config.h"
#include "coroutine.h"

CO_FN_BEGIN(example_task1, int, arg)
{
    static co_event_t event;

    while (true) {
        printf("example_task1, arg: %d\n", arg);

        pi_task_push_delayed_us(co_event_init(&event), 1000000);
        CO_WAIT(&event);
    }
}
CO_FN_END()

CO_FN_DECLARE(example_task3);

CO_FN_BEGIN(example_task2, void *, _arg)
{
    static co_fn_ctx_t ctx3;
    static co_event_t task3_done;

    while (true) {
        printf("example_task2\n");

        co_fn_push_start(&ctx3, example_task3, NULL, co_event_init(&task3_done));
        CO_WAIT(&task3_done);
    }
}
CO_FN_END()

CO_FN_BEGIN(example_task3, void *, _arg)
{
    static co_event_t event;
    static int counter;

    pi_perf_conf(1<<PI_PERF_CYCLES);
    pi_perf_start();
    pi_perf_reset();

    counter = 0;

    while (counter < 5) {
        // Example of local variables usage
        int delta_clk = pi_perf_read(PI_PERF_CYCLES);
        pi_perf_reset();

        // OK: delta_clock does not cross a suspension point
        printf("example_task3 %d (%d cycles)\n", counter, delta_clk);
        counter += 1;

        pi_task_push_delayed_us(co_event_init(&event), 1000000);
        CO_WAIT(&event);

        // ERROR: the coroutine was suspended by CO_WAIT, delta_clk is not 
        // accessible anymore
        // delta_clk += 1;
        // printf("+1 (%d cycles)\n", delta_clk);
    }

    printf("example_task3 DONE\n");
}
CO_FN_END()

void main_task() {
    int arg = 1234;
    co_fn_ctx_t ctx1;
    co_fn_push_start(&ctx1, example_task1, (void *)arg, NULL);

    co_fn_ctx_t ctx2;
    co_fn_push_start(&ctx2, example_task2, NULL, NULL);

    while (true) {
		pi_yield();
	}
}

#if defined(__PULP_OS__)
#include "pulp.h"
#define pi_fll_get_frequency(x)       ( rt_freq_get(x) )
#define pi_fll_set_frequency(x, y, z) ( rt_freq_set(x, y) )
#define pi_pmu_set_voltage(x, y)      ( rt_voltage_force(RT_VOLTAGE_DOMAIN_MAIN, x, NULL) )
#endif

int main(void) {
    pi_fll_set_frequency(FLL_SOC, 100000000, 1);

    printf("\n\n\t *** PMSIS Kickoff ***\n\n");
    return pmsis_kickoff((void *)main_task);
}

/*********** Coroutines after preprocessor macro expansion ***************/
// This example is slightly out of date compared to the actual 
// implementation of coroutine.h, but the overall meaning is still the same
// static void _example_task(co_fn_ctx_t *ctx) {
//     static co_event_t event;

// // Start of CO_FN_BEGIN(ctx)
//     /* Ensure -Wmaybe-uninitialized is always an error to catch local */
//     /* variables used across suspension points. */
//     _Pragma("GCC diagnostic push")
//     _Pragma("GCC diagnostic error \"-Wmaybe-uninitialized\"")

//     co_fn_ctx_t *__co_ctx = ctx;
//     co_fn_resume_t __co_resume = __co_ctx->resume_point;
//     __co_ctx->resume_point = CO_RESUME_RUNNING;

//     CO_VERBOSE_PRINT(
//         "%s, ctx: %p, resume_point: %d\n",
//         __FUNCTION__, ctx, __co_resume
//     );

//     switch (__co_resume) {
//     case CO_RESUME_START:
// // End of CO_FN_BEGIN()

//         /* empty statement */;

//         while (true) {
//             pi_task_push_delayed_us(co_event_init(&event), 100000);

// // Start of CO_WAIT(&event)
//             co_event_wait(&event, co_fn_suspend(ctx, 1));
//             return;

//     case 1:
// // End of CO_WAIT(&event)

//             printf("asd\n");
//         }

// // Start of CO_FN_END()
//     CO_RETURN();

//     case CO_RESUME_RUNNING:
//         CO_ASSERTION_FAILURE(
//             "Function resumed without being properly suspended first.\n"
//         );

//     case CO_RESUME_DONE:
//         CO_ASSERTION_FAILURE(
//             "Function resumed after having returned.\n"
//         );

//     default:
//         CO_ASSERTION_FAILURE(
//             "Function resumed from invalid resume point %d.\n",
//             __co_resume
//         );
//     }

//     _Pragma("GCC diagnostic pop")
// // End of CO_FN_END()

// }
