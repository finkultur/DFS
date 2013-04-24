/*
 */

#include <stdio.h>
#include <unistd.h>

#include <arch/cycle.h>
#include <tmc/cpus.h>
#include <tmc/task.h>

#include <asm/smp.h>

#include "perfcount.h"

int main(void) {

    cpu_set_t cpus;
    int wr_cnt, wr_miss, drd_cnt, drd_miss;

    unsigned long start_cycles = get_cycle_count();

    // Init cpus
    if (tmc_cpus_get_my_affinity(&cpus) != 0) {
        tmc_task_die("Failure in 'tmc_cpus_get_my_affinity()'.");
    }
    int num_cpus = tmc_cpus_count(&cpus);
    printf("cpus_count is: %i\n", num_cpus);

    // Setup Counters
    setup_all_counters(&cpus);



/*
    unsigned long start_for = get_cycle_count();
    unsigned long cycles[num_cpus];

    for (int i=0;i<num_cpus;i++) {
        if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(&cpus, i)) < 0) {
                tmc_task_die("failure in 'tmc_set_my_cpu'");
        }
        printf("cpu %i\n", i);
        cycles[i] = get_cycle_count();
    }
    unsigned long end_for = get_cycle_count();

    for (int i=1;i<num_cpus;i++) {
        unsigned long temp = cycles[i] - cycles[i-1];
        printf("time between %i and %i is %lu\n", i-1, i, temp);        
    }
*/
    return 0;
}
