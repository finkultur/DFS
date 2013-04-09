/*
 * sched_algs.c
 *
 * Different scheduling algorithms
 *
 */

// Tilera
#include <tmc/cpus.h>
#include <tmc/task.h>

#include "perfcount.h"
#include "sched_algs.h"

/*
 * Returns a suitable tile.
 */
/*int get_tile(cpu_set_t *cpus, int *tileAlloc) {
    int best_tile;
    if (best_tile = get_empty_tile(cpus, tileAlloc) >= 0) {
        return best_tile;
    }
    else {
        return get_tile_with_min_write_miss_rate(cpus);
    }
}*/
int get_tile(cpu_set_t *cpus, int *tileAlloc) {
    int num_of_cpus = tmc_cpus_count(cpus);

    for (int i=0;i<num_of_cpus;i++) {
        if (tileAlloc[i] == 0) {
            return i;
        }
    }
    return get_tile_with_min_write_miss_rate(cpus);
}

/*
 * Tries to get an empty tile, given a pointer to a set of cpus.
 * Returns -1 if no tile is empty.
 */
int get_empty_tile(cpu_set_t *cpus, int *tileAlloc) {
    int num_of_cpus = tmc_cpus_count(cpus);

    for (int i=0;i<num_of_cpus;i++) {
        if (tileAlloc[i] == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Given a a pointer to a set of cpus, iterate through all of them to read
 * performance counters. Returns the number of the "best" tile - defined here
 * as the one with least miss rate on writes to the data cache.
 */
int get_tile_with_min_write_miss_rate(cpu_set_t *cpus) {
    
    int num_of_cpus = tmc_cpus_count(cpus);
    int best_tile = 0;
    float min_value = 1.0;
    int wr_miss, wr_cnt, drd_miss, drd_cnt;
    float wr_miss_rate, drd_miss_rate;

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
    }
    return best_tile;
}
