/*
 * sched_algs.c
 *
 * Different scheduling algorithms
 *
 */

// Just for debugging
#include <stdio.h>

// Tilera
#include <tmc/cpus.h>
#include <tmc/task.h>

#include "perfcount.h"
#include "proc_table.h"
#include "sched_algs.h"


int get_tile(cpu_set_t *cpus, proc_table table) {
    return get_tile_from_counters(cpus, table);
}
/*
 * Returns a suitable tile.
 *
 * Tries to get an empty tile, otherwise find the tile with least data cache
 * write miss rate.
 */
int get_tile_by_miss_rate(cpu_set_t *cpus, proc_table table, float *wr_miss_rates) {
    int num_of_cpus = tmc_cpus_count(cpus);
    printf("get_tile: got cpu count %i\n", num_of_cpus);
    // If a tile it empty, its probably the most suitable tile...
    int empty_tile = get_empty_tile(num_of_cpus, table);
    if (empty_tile >= 0) {
        return empty_tile;
    }

    // Otherwise, calculate the best tile through multiplying the tile's miss
    // rate by the number of processes running on that tile. 
    int best_tile = 0;
    float min_val = get_pid_count(table, 0) * wr_miss_rates[0];
    int temp_val;
    for (int i=1;i<num_of_cpus;i++) {
        temp_val = get_pid_count(table, i) * wr_miss_rates[i]; 
        if (temp_val < min_val) {
            best_tile = i;
            min_val = temp_val; 
        }
    }
    // Return the tile with the lowest value (calculated above)
    return best_tile;
}

/*
 * Returns the tile with the least amount of contention. If there is
 * an empty tile, it will return that and skip all other calculations.
 */
int get_tile_from_counters(cpu_set_t *cpus, proc_table table) {
    int num_of_cpus = tmc_cpus_count(cpus);
    printf("get_tile_from_counters: CPU COUNT %i\n", num_of_cpus);
    // If a tile it empty, its probably the most suitable tile...
    int empty_tile = get_empty_tile(num_of_cpus, table);
    if (empty_tile >= 0) {
        return empty_tile;
    }

    int best_tile = 0;
    int min_val = table->miss_counters[0]; 
    for (int i=1;i<num_of_cpus;i++) {
        if (table->miss_counters[i] == 0) {
            return i;
        }
        else if (table->miss_counters[i] < min_val) {
            best_tile = i;
            min_val = table->miss_counters[i];
        } 
    }
    return best_tile;
}

/*
 * Returns an empty tile (if there is one).
 * Returns -1 if no tile is empty.
 */
int get_empty_tile(int num_of_cpus, proc_table table) {
    for (int i=0;i<num_of_cpus;i++) {
        if (get_pid_count(table, i) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Returns the tile with least processes running.
 */
int get_least_occupied_tile(int num_of_cpus, proc_table table) {
    int best_tile = 0;
    int min_val = get_pid_count(table, 0);

    for (int i=1;i<num_of_cpus;i++) {
        if (get_pid_count(table, i) < min_val) {
            best_tile = i;
            min_val = get_pid_count(table, i); 
        }
    }
    return best_tile;
}

/*
 * Given a pointer to a set of cpus, it iterates through all of them to read
 * performance counters. Returns the number of the "best" tile - defined here
 * as the one with least miss rate on writes to the data cache.
 */
int get_tile_with_min_write_miss_rate(cpu_set_t *cpus) {
    
    int num_of_cpus = tmc_cpus_count(cpus);
    int best_tile = 0;
    float min_value = 1.0;
    int wr_miss, wr_cnt, drd_miss, drd_cnt;
    float wr_miss_rate;
    //float drd_miss_rate;

    for (int i=0;i<num_of_cpus;i++) {
        if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(cpus, i)) < 0) {
            tmc_task_die("failure in 'tmc_set_my_cpu'");
        }
        read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
        if (wr_cnt != 0) {
            wr_miss_rate = ((float) wr_miss) / wr_cnt;
        }
        else {
            wr_miss_rate = 0.0;
        }
        /*if (drd_cnt != 0) {
            drd_miss_rate = ((float) drd_miss) / drd_cnt;
        }
        else {
            drd_miss_rate = 0.0;
        }*/
        // Set the best tile to the one with lowest cache write miss rate.
        if (wr_miss_rate < min_value) {
                min_value = wr_miss_rate;
                best_tile = i;
        }
        printf("Tile %i: Wr_miss_rate %f, wr_miss %i, wr_cnt %i\n", i, wr_miss_rate, wr_miss, wr_cnt);
    }
    return best_tile;
}

/**
 * Scans the array updated by the poll_pmc-thread for the lowest
 * write cache miss rate and returns that tile.
 */
int get_tile_from_wr_miss_array(int num_of_cpus, float *wr_miss_rates) {
    int best_tile = 0;
    float min_val = 1.0;

    for (int i=0;i<num_of_cpus;i++) {
        if (wr_miss_rates[i] < min_val) {
            min_val = wr_miss_rates[i];
            best_tile = i;
        }
    }
    return best_tile;
}
