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

#include "pid_table.h"
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

/*
 * "Thread-function" that polls the performance registers every 
 * POLLING_INTERVAL seconds.
 *
 * Takes a struct containing the needed arguments:
 * - a pointer to a cpu_set_t
 * - an array of floats where it saves write miss rates
 * - an array of floats where it saves read miss rates
 *
 */
/*void *poll_pmcs(void *struct_with_all_args) {
    cpu_set_t *cpus;
    int num_of_cpus;
    int wr_miss, wr_cnt, drd_miss, drd_cnt;

    struct poll_thread_struct *data;
    data = (struct poll_thread_struct *) struct_with_all_args;
    cpus = data->cpus;
    float *wr_miss_rates = data->wr_miss_rates;
    float *drd_miss_rates = data->drd_miss_rates;
    num_of_cpus = tmc_cpus_count(cpus);

    pid_table table = data->pidtable;
    int *tileAlloc = data->tileAlloc; 

    while(1) {
        for(int i=0;i<num_of_cpus;i++) {
            if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(cpus, i)) < 0) {
                tmc_task_die("failure in 'tmc_set_my_cpu'");
                return (void*) -1;
            }
            read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
            if (wr_cnt != 0) {
                wr_miss_rates[i] = ((float) wr_miss) / wr_cnt;
            }
            if (drd_cnt != 0) {
                drd_miss_rates[i] = ((float) drd_miss) / drd_cnt;
            }
        }
        sleep(POLLING_INTERVAL);
    }
}

int migrate_process(cpu_set_t *cpus, pid_table table, int *tileAlloc, int pid, int oldtile, int newtile) {
    //int oldtile = get_tile_num(table, pid);

    // set pid to new cpu
    tmc_cpus_set_task_cpu((tmc_cpus_find_nth_cpu(cpus, newtile)), pid);

    // Reorder pid_table
    set_tile_num(table, pid, newtile);

    // Reorder tile allocation array
    tileAlloc[oldtile]--;
    tileAlloc[newtile]++;

    return 0;
}
*/
