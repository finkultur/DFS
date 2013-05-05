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

#include "proc_table.h"
#include "perfcount.h"
#include "sched_algs.h"
#include "migrate.h"

#define POLLING_INTERVAL 10

cpu_set_t *cpus_ptr;
int num_of_cpus;
float *write_miss_rates;
float *read_miss_rates;

/*
 * "Thread-function" that polls the performance registers every
 * POLLING_INTERVAL seconds.
 *
 * Takes a struct containing the needed arguments:
 * - a pointer to a cpu_set_t
 * - an array of floats where it saves write miss rates
 * - an array of floats where it saves read miss rates
 * - an array of ints with pids per tile
 * - a proc_table struct
 */
void *poll_pmcs(void *struct_with_all_args) {
    int wr_miss, wr_cnt, drd_miss, drd_cnt;
    float all_misses, wr_drd_cnt;
    proc_table table;

    struct poll_thread_struct *data;
    data = (struct poll_thread_struct *) struct_with_all_args;

    table = data->proctable;
    write_miss_rates = data->wr_miss_rates;
    read_miss_rates = data->drd_miss_rates;
    cpus_ptr = data->cpus;

    num_of_cpus = tmc_cpus_count(cpus_ptr);
    printf("\nNUMBER OF CPUS: %i\n", num_of_cpus);

    // Setup all performance counters on every initialized tile
    if (setup_all_counters(cpus_ptr) != 0) {
        printf("setup_all_counters failed\n");
        return (void *) -1;
    }

    // Read counters and update table every POLLING_INTERVAL seconds
    while(1) {
        for(int i=0;i<num_of_cpus;i++) {
            // Switch to tile i
            if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(cpus_ptr, i)) < 0) {
                tmc_task_die("failure in 'tmc_set_my_cpu'");
                return (void*) -1;
            }
            // Read counters
            read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);

            // Update miss counters. Gives the new miss rate to miss_count.
            all_misses = wr_miss+drd_miss;
            wr_drd_cnt = wr_cnt + drd_cnt;
			modify_miss_count(table, i, all_misses/wr_drd_cnt);

            clear_counters();
        }
        check_for_possible_migration(table);
        sleep(POLLING_INTERVAL);
    }

}

/*
 * Only migrate processes if a tile has a miss-count-value higher than
 * two times the average miss-count-value.
 */
void check_for_possible_migration(proc_table table) {
    float miss_cnt;

    // Debugging - just print values from table
    /*printf("lolgrate: avg_miss_count = %f\n", table->avg_miss_rate);
    printf("lolgrate: processes/miss_cnt: ");
    for (int i=0;i<table->num_tiles;i++) {
        printf("%i/%f ", i, table->miss_counters[i]);
    }
    printf("\n");
	*/
    // 1.5 and 2 are magic values I just made up
    // They should most certainly be changed in some way.
    for (int i=0;i<table->num_tiles;i++) {
        miss_cnt = table->miss_counters[i];
        // Cool down tile if miss rate is reasonably and the tile
        // has enough processes to migrate.
        if (miss_cnt > (1.5*table->avg_miss_rate) && (get_pid_count(table, i) > 1)) {
            //printf("pid count for tile %i is %i", i, get_pid_count(table, i));
        	chill_it(table, i);
        }
    }
}

/**
 * Migrates the process with the smallest class value from a tile
 * to a new tile.
 */
void migrate_smallest(proc_table table, int tilenum) {
	int new_tile = get_tile(cpus_ptr, table);
	int pids_on_old = get_pid_count(table, tilenum);
	pid_t pids_to_move[pids_on_old];
	get_pid_vector(table, tilenum, pids_to_move, pids_on_old);

	pid_t smallest_pid = pids_to_move[0];
	int min_val = get_class(table, pids_to_move[0]);
	for (int i=1; i<pids_on_old; i++) {
		if (get_class(table, pids_to_move[i]) < min_val) {
			smallest_pid = pids_to_move[i];
			min_val = get_class(table, pids_to_move[i]);
		}
	}

	migrate_process(table, smallest_pid, new_tile);
}



/*
 * Moves processes from a tile to a new one until their total class values are
 * moderately balanced.
 */
void chill_it(proc_table table, int tilenum) {

	int new_tile = get_tile(cpus_ptr, table);
    pid_t pids_to_move[1];
    get_pid_vector(table, tilenum, pids_to_move, 1);
    
    migrate_process(table, pids_to_move[0], new_tile);

/*	int from_val = get_total_value_of_classes(table, tilenum);
	int to_val = get_total_value_of_classes(table, new_tile);
	int diff = from_val-to_val;

	int pids_on_old = get_pid_count(table, tilenum);
	pid_t pids_to_move[pids_on_old];
	get_pid_vector(table, tilenum, pids_to_move, pids_on_old);

	// This is probably not a good way to do it

	if (diff < 0) {
		// what to do?
		return; // ???
	}

	// 3 is a magic value I just made up
	for (int i=0; diff > 3 && i<pids_on_old; i++) {
		if (get_class(table, pids_to_move[i]) <= diff) {
			migrate_process(table, pids_to_move[i], new_tile);
		}
	}
*/
}

/*
 * Moves a number of processes from a specified tile to new ones with less
 * contention.
 */
void cool_down_tile(proc_table table, int tilenum, int how_much) {
    pid_t pids_to_move[how_much];
    get_pid_vector(table, tilenum, pids_to_move, how_much);
    int new_tile;

    // Move the pids
    for (int i=0;(i<how_much && i<get_pid_count(table, tilenum));i++) {
        if (pids_to_move[i] > 0) { // Redundant check?
            new_tile = get_tile(cpus_ptr, table);
            migrate_process(table, pids_to_move[i], new_tile);
        }
    }
}

/*
 * Moves a process a new tile.
 */
void migrate_process(proc_table table, int pid, int newtile) {
    int oldtile = get_tile_num(table, pid);
    // set pid to new cpu
    //printf("migrate_process: NUMBER OF CPUS is %i\n", tmc_cpus_count(cpus_ptr));
    if (tmc_cpus_set_task_cpu((tmc_cpus_find_nth_cpu(cpus_ptr, newtile)), pid) < 0) {
        tmc_task_die("Failure in tmc_cpus_set_task_cpu (in migrate_process)");
    }
    
    // Reorder proc_table
    move_pid_to_tile(table, pid, newtile);

    printf("Pid %i moved from logical tile %i to logical tile %i\n",
           pid, oldtile, newtile);
}

