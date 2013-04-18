/*
 * migrate.c
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arch/cycle.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "pid_table.h"
#include "perfcount.h"
#include "sched_algs.h"
#include "migrate.h"

#define POLLING_INTERVAL 10
#define CONTENTION_LIMIT 0.58 // Totally arbitrary
#define READS_BEFORE_RESET 3 // Reset counters every THIS*POLLING_INTERVAL seconds

cpu_set_t *cpus;
int num_of_cpus;
float *write_miss_rates;
float *read_miss_rates;
pid_table pidtable;
int *pids_per_tile;

/*
 * "Thread-function" that polls the performance registers every 
 * POLLING_INTERVAL seconds.
 *
 * Takes a struct containing the needed arguments:
 * - a pointer to a cpu_set_t
 * - an array of floats where it saves write miss rates
 * - an array of floats where it saves read miss rates
 * - an array of ints with pids per tile
 * - a pid_table struct
 */
void *poll_pmcs(void *struct_with_all_args) {
    int wr_miss, wr_cnt, drd_miss, drd_cnt;
    int physical_tile;

    struct poll_thread_struct *data;
    data = (struct poll_thread_struct *) struct_with_all_args;

    pidtable = data->pidtable;
    pids_per_tile = data->tileAlloc;
    write_miss_rates = data->wr_miss_rates;
    read_miss_rates = data->drd_miss_rates;
    cpus = data->cpus;

    num_of_cpus = tmc_cpus_count(cpus);
    printf("\nNUMBER OF CPUS: %i\n", num_of_cpus);

    // Setup all performance counters on every initialized tile
    if (setup_all_counters(cpus) != 0) {
        printf("setup_all_counters failed\n");
        return (void *) -1;
    }


    int reset = READS_BEFORE_RESET;
    // Read counters and update table every X seconds
    while(1) {
        for(int i=0;i<num_of_cpus;i++) {
            // Switch to tile i
            if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(cpus, i)) < 0) {
                tmc_task_die("failure in 'tmc_set_my_cpu'");
                return (void*) -1;
            }
            // Read counters
            read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
            physical_tile = tmc_cpus_get_my_current_cpu();
            printf("Logical/Physical Tile %i/%i:", i, physical_tile); 
            printf("wr_miss %i, wr_cnt %i, drd_miss %i, drd_cnt %i\n",
                   wr_miss, wr_cnt, drd_miss, drd_cnt);
            if (wr_cnt != 0) {
                write_miss_rates[i] = ((float) wr_miss) / wr_cnt;
                // If value is over CONTENTION_LIMIT, "cool down" tile
                if ((pids_per_tile[i] * write_miss_rates[i]) > CONTENTION_LIMIT) {
                    printf("Cooool down please!\n");
                    cool_down_tile(i);
                }
            }
            if (drd_cnt != 0) {
                read_miss_rates[i] = ((float) drd_miss) / drd_cnt;
            }
        }
        // We reset all performance counters every x iteration
        reset--;
        if (reset == 0) {
            printf("Resetting performance counters.\n");
            clear_counters();
            reset = READS_BEFORE_RESET;
        }
        print_wr_miss_rates();
        sleep(POLLING_INTERVAL);
    }

}

void print_wr_miss_rates() {

    printf("Wr miss rates:\n");
    for (int i=0;i<num_of_cpus;i++) {
        printf("Tile %i: Processes %i, Write miss rate is: %f\n", 
               i, pids_per_tile[i], write_miss_rates[i]);
    }

}

/*
 * Moves an arbitrary process from a specified tile to a new one with less
 * contention. 
 */
void cool_down_tile(int tilenum) {
    int pid_to_move = get_first_pid_from_tile(tilenum);
    if (pid_to_move > 0) {
        int newtile = get_tile(cpus, pids_per_tile, write_miss_rates);
        migrate_process(pid_to_move, newtile);
    }
}

/*
 * Moves a process a new tile.
 */
void migrate_process(int pid, int newtile) {
    int oldtile = get_tile_num(pidtable, pid);
    // set pid to new cpu
    tmc_cpus_set_task_cpu((tmc_cpus_find_nth_cpu(cpus, newtile)), pid);
    // Reorder pid_table
    set_tile_num(pidtable, pid, newtile);
    // Reorder tile allocation array
    pids_per_tile[oldtile]--;
    pids_per_tile[newtile]++;
    printf("Pid %i moved from logical tile %i to logical tile %i\n", 
           pid, oldtile, newtile);
}

/*
 * THIS CODE IS RETARDED
 */
int get_first_pid_from_tile(int tile_num) {
    for (int i=650;i<720;i++) {
        if (tile_num == get_tile_num(pidtable, i)) {
            return i;
        }
    }
    return -1;
}
