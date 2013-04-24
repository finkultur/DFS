/*
 * perfcount.c
 */

#include <stdio.h> // Just for debugging
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arch/cycle.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "perfcount.h"

void
setup_counters(int event1, int event2, int event3, int event4)
{
  __insn_mtspr(SPR_PERF_COUNT_CTL, event1 | (event2 << 16));
  __insn_mtspr(SPR_AUX_PERF_COUNT_CTL, event3 | (event4 << 16));
}

void
read_counters(int* event1, int* event2, int* event3, int* event4)
{
  *event1 = __insn_mfspr(SPR_PERF_COUNT_0);
  *event2 = __insn_mfspr(SPR_PERF_COUNT_1);
  *event3 = __insn_mfspr(SPR_AUX_PERF_COUNT_0);
  *event4 = __insn_mfspr(SPR_AUX_PERF_COUNT_1);
}

void
clear_counters()
{
  __insn_mtspr(SPR_PERF_COUNT_0, 0);
  __insn_mtspr(SPR_PERF_COUNT_1, 0);
  __insn_mtspr(SPR_AUX_PERF_COUNT_0, 0);
  __insn_mtspr(SPR_AUX_PERF_COUNT_1, 0);
}

/*
 * Setup counters for all tiles in cpu set
 */ 
int setup_all_counters(cpu_set_t *cpus) {
    int num_of_cpus = tmc_cpus_count(cpus);
    for (int i=0;i<num_of_cpus;i++) {
        if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(cpus, i)) < 0) {
            tmc_task_die("failure in 'tmc_set_my_cpu'");
            return -1;
        }
        clear_counters();
        setup_counters(LOCAL_WR_MISS, LOCAL_WR_CNT, LOCAL_DRD_MISS, LOCAL_DRD_CNT);
    }
    return 0; 
}
