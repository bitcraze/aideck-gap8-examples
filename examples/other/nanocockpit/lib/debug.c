/*
 * debug.c
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

#include "debug.h"

#include "time.h"

#include <rt/rt_api.h>

void memory_dump(pi_device_t *cluster) {
    // TODO: use pi_*_malloc_dump after updating to GAP SDK 3.9.1+
    rt_user_alloc_dump(rt_alloc_l2()); // L2
    rt_user_alloc_dump(rt_alloc_fc_tcdm()); // FC L1

    if (cluster) {
        rt_user_alloc_dump(rt_alloc_l1(0)); // CL L1
    }
}

#define DBG_UNIT_CL_BASE        0x10300000
#define DBG_UNIT_CL_CORE_OFFSET     0x8000

#define DBG_UNIT_CTRL               0x0000
#define DBG_UNIT_HIT                0x0004
#define DBG_UNIT_IE                 0x0008
#define DBG_UNIT_CAUSE              0x000C

#define DBG_UNIT_GPR0               0x0400
#define DBG_UNIT_GPR1               0x0404
#define DBG_UNIT_GPR2               0x0408
#define DBG_UNIT_GPR3               0x040C
#define DBG_UNIT_GPR4               0x0410
#define DBG_UNIT_GPR5               0x0414
#define DBG_UNIT_GPR6               0x0418
#define DBG_UNIT_GPR7               0x041C
#define DBG_UNIT_GPR8               0x0420
#define DBG_UNIT_GPR9               0x0424
#define DBG_UNIT_GPR10              0x0428
#define DBG_UNIT_GPR11              0x042C
#define DBG_UNIT_GPR12              0x0430
#define DBG_UNIT_GPR13              0x0434
#define DBG_UNIT_GPR14              0x0438
#define DBG_UNIT_GPR15              0x043C
#define DBG_UNIT_GPR16              0x0440
#define DBG_UNIT_GPR17              0x0444
#define DBG_UNIT_GPR18              0x0448
#define DBG_UNIT_GPR19              0x044C
#define DBG_UNIT_GPR20              0x0450
#define DBG_UNIT_GPR21              0x0454
#define DBG_UNIT_GPR22              0x0458
#define DBG_UNIT_GPR23              0x045C
#define DBG_UNIT_GPR24              0x0460
#define DBG_UNIT_GPR25              0x0464
#define DBG_UNIT_GPR26              0x0468
#define DBG_UNIT_GPR27              0x046C
#define DBG_UNIT_GPR28              0x0470
#define DBG_UNIT_GPR29              0x0474
#define DBG_UNIT_GPR30              0x0478
#define DBG_UNIT_GPR31              0x047C

#define DBG_UNIT_NPC                0x2000
#define DBG_UNIT_PPC                0x2004

#define DBG_UNIT_CSR_UHARTID        0x4050
#define DBG_UNIT_CSR_MSTATUS        0x4C00
#define DBG_UNIT_CSR_MTVEC          0x4C14
#define DBG_UNIT_CSR_MEPC           0x4D04
#define DBG_UNIT_CSR_MCAUSE         0x4D08

#define DBG_UNIT_CSR_PCCR           0x5E00
#define DBG_UNIT_CSR_PCER           0x5E80
#define DBG_UNIT_CSR_PCMR           0x5E84

#define DBG_UNIT_CSR_HWLP0S         0x5EC0
#define DBG_UNIT_CSR_HWLP0E         0x5EC4
#define DBG_UNIT_CSR_HWLP0C         0x5EC8
#define DBG_UNIT_CSR_HWLP1S         0x5ED0
#define DBG_UNIT_CSR_HWLP1E         0x5ED4
#define DBG_UNIT_CSR_HWLP1C         0x5ED8

#define DBG_UNIT_CSR_PRIVLV         0x7040
#define DBG_UNIT_CSR_MHARTID        0x7C50

#define DBG_UNIT_CL_READ32(core_id, register) \
    pulp_read32(DBG_UNIT_CL_BASE + core_id * DBG_UNIT_CL_CORE_OFFSET + register)

#define DBG_UNIT_CL_WRITE32(core_id, register, value) \
    pulp_write32(DBG_UNIT_CL_BASE + core_id * DBG_UNIT_CL_CORE_OFFSET + register, value)

static const char *register_names[32] = {
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "s0",
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6",
};

static uint32_t read_instr(void *pc) {
    uint32_t instr = *((uint32_t *)pc);

    // Check whether it's a compressed instruction
    uint8_t rvc_opcode = (instr >> 0) & ((1 << 2) - 1);
    bool    is_rvc     = (rvc_opcode != 0x3);
    if (is_rvc) {
        instr = instr & ((1 << 16) - 1);
    }

    return instr;
}

void cluster_core_dbg_dump(int core_id, bool halt) {
    printf("CLUSTER CORE %d CORE DUMP\n", core_id);
    printf("=========================\n");

    uint32_t ctrl  = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_CTRL);
    uint32_t hit   = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_HIT);
    uint32_t ie    = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_IE);
    uint32_t cause = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_CAUSE);

    uint8_t ctrl_halt_status = (ctrl >> 16) & 0x1;
    uint8_t ctrl_sste        = (ctrl >>  0) & 0x1;
    printf("CTRL  = 0x%08x (HALT_STATUS %d, SSTE %d)\n", ctrl, ctrl_halt_status, ctrl_sste);
    
    uint8_t hit_sleep = (hit >> 16) & 0x1;
    uint8_t hit_ssth  = (hit >>  0) & 0x1;
    printf("HIT   = 0x%08x (SLEEP %d, SSTH %d)\n", hit, hit_sleep, hit_ssth);
    
    uint8_t ie_ecall     = (hit >> 11) & 0x1;
    uint8_t ie_elsu_dup  = (hit >>  7) & 0x1;
    uint8_t ie_elsu      = (hit >>  5) & 0x1;
    uint8_t ie_ebrk      = (hit >>  3) & 0x1;
    uint8_t ie_eill      = (hit >>  2) & 0x1;
    printf(
           "IE    = 0x%08x (ECALL %d, ELSU_DUP %d, ELSU %d, EBRK %d, EILL %d)\n",
            ie, ie_ecall, ie_elsu_dup, ie_elsu, ie_ebrk, ie_eill
    );

    uint8_t cause_irq    = (cause >> 31) & 0x1;
    uint8_t cause_cause  = (cause >>  0) & ((1 << 5) - 1);
    printf("CAUSE = 0x%08x (IRQ %d, CAUSE 0x%02x)\n", cause, cause_irq, cause_cause);

    printf("=========================\n");

    uint32_t saved_ctrl;
    if (halt) {
        if (!ctrl_halt_status) {
            saved_ctrl = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_CTRL);
            uint32_t new_ctrl = saved_ctrl | (1 << 16);
            DBG_UNIT_CL_WRITE32(core_id, DBG_UNIT_CTRL, new_ctrl);
        }

        for (int i = 0; i < 32; i++) {
            uint32_t value = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_GPR0 + i * 0x4);
            int32_t value_i32 = *((int32_t *)&value);
            int16_t value_i16 = *((int16_t *)&value);
            int8_t value_i8 = *((int8_t *)&value);

            printf(
                "x%-2d (%s) = 0x%08x (u32 %u, i32 %d, i16 %hd, i8 %hhd)\n",
                i, register_names[i], 
                value, value, value_i32, value_i16, value_i8
            );
        }

        uint32_t npc = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_NPC);
        uint32_t ppc = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_PPC);

        uint32_t next_instr = read_instr((void *)npc);
        uint32_t prev_instr = read_instr((void *)ppc);

        printf("NPC = 0x%08x (opcode 0x%08x)\n", npc, next_instr);
        printf("PPC = 0x%08x (opcode 0x%08x)\n", ppc, prev_instr);

        uint32_t csr_uhartid = DBG_UNIT_CL_READ32(core_id, DBG_UNIT_CSR_UHARTID);

        uint8_t csr_cluster_id = (csr_uhartid >> 5) & ((1 << 6) - 1);
        uint8_t csr_core_id    = (csr_uhartid >> 0) & ((1 << 4) - 1);

        printf("CSR_UHARTID = 0x%08x (cluster_id %d, core_id %d)\n", csr_uhartid, csr_cluster_id, csr_core_id);

        printf("=========================\n");
    }

    printf("\n");

    if (halt & !ctrl_halt_status) {
        DBG_UNIT_CL_WRITE32(core_id, DBG_UNIT_CTRL, saved_ctrl);
    }
}

static uint32_t watchdog_last_reset;
static pi_task_t watchdog_task;

static void watchdog_core_dump(void *arg) {
    uint32_t watchdog_delta = time_get_us() - watchdog_last_reset;
    if (watchdog_delta > 2000000) {
        VERBOSE_PRINT("Watchdog expired, time since last reset %dms\n", watchdog_delta / 1000);
        
        for (int i = 0; i < 8; i++) {
            cluster_core_dbg_dump(i, /* halt */ true);
        }
    }

    pi_task_push_delayed_us(pi_task_callback(&watchdog_task, watchdog_core_dump, NULL), 100000);
}

void watchdog_reset() {
    watchdog_last_reset = time_get_us();
}

void watchdog_start() {
    watchdog_reset();
    pi_task_push(pi_task_callback(&watchdog_task, watchdog_core_dump, NULL));
}

/// MALLOC GUARDS

static const int malloc_guard_size = 128;
static uint8_t malloc_guard_pre = 0xaa;
static uint8_t malloc_guard_alloc = 0xbb;
static uint8_t malloc_guard_post = 0xcc;

void *pmsis_l1_malloc_guard(size_t size) {
  void *alloc_pre = pmsis_l1_malloc(malloc_guard_size + size + malloc_guard_size);
  
  if (!alloc_pre) {
    printf("[pmsis_l1_malloc_guard] alloc failed\n");
    pmsis_exit(-1);
  }

  void *alloc = alloc_pre + malloc_guard_size;
  void *alloc_post = alloc + size;

  memset(alloc_pre, malloc_guard_pre, malloc_guard_size);
  memset(alloc, malloc_guard_alloc, size);
  memset(alloc_post, malloc_guard_post, malloc_guard_size);

//   printf("[pmsis_l1_malloc_guard] %dB @ L2, 0x%08x (pre 0x%02x x %d, post 0x%02x x %d)\n", size, alloc, malloc_guard_pre, malloc_guard_size, malloc_guard_post, malloc_guard_size);
  return alloc;
}

void pmsis_l1_malloc_guard_free(void *alloc, size_t size) {
  malloc_guard_check(alloc, size);
  
  uint8_t *alloc_pre = alloc - malloc_guard_size;
  pmsis_l1_malloc_free(alloc_pre, malloc_guard_size + size + malloc_guard_size);
}

void *pi_l2_malloc_guard(size_t size) {
  void *alloc_pre = pi_l2_malloc(malloc_guard_size + size + malloc_guard_size);

  if (!alloc_pre) {
    printf("[pi_l2_malloc_guard] alloc failed\n");
    pmsis_exit(-1);
  }

  void *alloc = alloc_pre + malloc_guard_size;
  void *alloc_post = alloc + size;

  memset(alloc_pre, malloc_guard_pre, malloc_guard_size);
  memset(alloc, malloc_guard_alloc, size);
  memset(alloc_post, malloc_guard_post, malloc_guard_size);

  printf("[pi_l2_malloc_guard] %dB @ L2, 0x%08x (pre 0x%02x x %d, post 0x%02x x %d)\n", size, alloc, malloc_guard_pre, malloc_guard_size, malloc_guard_post, malloc_guard_size);
  return alloc;
}

void pi_l2_malloc_guard_free(void *alloc, size_t size) {
  malloc_guard_check(alloc, size);

  uint8_t *alloc_pre = alloc - malloc_guard_size;
  pi_l2_free(alloc_pre, malloc_guard_size + size + malloc_guard_size);
}

void *malloc_guard_check(void *alloc, size_t size) {
  uint8_t *alloc_pre = alloc - malloc_guard_size;
  uint8_t *alloc_post = alloc + size;

  for (int i = 0; i < malloc_guard_size; i++) {
    if (alloc_pre[i] != malloc_guard_pre) {
      printf("Corrupted pre guard!\n");
      pmsis_exit(-1);
    }
  }

  for (int i = 0; i < malloc_guard_size; i++) {
    if (alloc_post[i] != malloc_guard_post) {
      printf("Corrupted post guard!\n");
      pmsis_exit(-1);
    }
  }
}
