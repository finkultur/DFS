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
#include "tile_table.h"
#include "sched_algs.h"

#define NUM_OF_MEMORY_DOMAINS 4

int get_tile(int tile_count, tile_table_t *table) {
    return get_tile_by_miss_rate(tile_count, table);
}

/*
 * Checks tile table for an empty cluster or the cluster
 * with least miss rate.
 */
int get_cluster_with_min_contention(tile_table_t *table) {
    int empty_cluster = get_empty_cluster(table);
    if (empty_cluster >= 0) {
        return empty_cluster;
    }

    int best_cluster = 0;
    int min_val = tile_table_get_cluster_miss_rate(table, 0);
    int new_val;
    for (int i=0;i<NUM_OF_MEMORY_DOMAINS;i++) {
        new_val = tile_table_get_cluster_miss_rate(i);
        if (new_val < min_val) {
            min_val = new_val;
            best_cluster = i;
        }
    }
    return best_cluster;
}

/*
 * Returns the index of an empty cluster if there is one.
 * If no cluster is empty, return -1.
 */
int get_empty_cluster(tile_table_t *table) {
    for (int i=0;i<NUM_OF_MEMORY_DOMAINS;i++) {
        if (get_cluster_pid_count(table, i) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Returns a suitable tile by class value.
 * Tries to get an empty tile, otherwise it returns
 * the tile with the lowest total class value.
 */
int get_tile_by_classes(int tile_count, tile_table_t *table) {
    //printf("get_tile: got cpu count %i\n", num_of_cpus);
    // If a tile it empty, its probably the most suitable tile...
    int empty_tile = get_empty_tile(tile_count, table);
    if (empty_tile >= 0) {
        return empty_tile;
    }

    int best_tile = 0;
    int min_val = tile_table_get_class_value(table, 0);
    for (int i=1;i<tile_count;i++) {
        if (tile_table_get_class_value(table, i) < min_val) {
            best_tile = i;
            min_val = tile_table_get_class_value(table, i);
        }
    }
    // Return the tile with the lowest value (calculated above)
    return best_tile;
}
/*
 * Returns a suitable tile.
 *
 * Tries to get an empty tile, otherwise find the tile with least data cache
 * write miss rate.
 */
int get_tile_by_miss_rate(int tile_count, tile_table_t *table) {
    // If a tile it empty, its probably the most suitable tile...
    int empty_tile = get_empty_tile(tile_count, table);
    if (empty_tile >= 0) {
        return empty_tile;
    }

    int best_tile = 0;
    float min_val = tile_table_get_miss_rate(table, 0);
    float temp_val;
    for (int i=1;i<tile_count;i++) {
        temp_val = tile_table_get_miss_rate(table, i);
        if (temp_val < min_val) {
            best_tile = i;
            min_val = temp_val;
        }
    }
    // Return the tile with the lowest value (calculated above)
    return best_tile;
}

/*
 * Returns an empty tile (if there is one).
 * Returns -1 if no tile is empty.
 */
int get_empty_tile(int num_of_cpus, tile_table_t *table) {
    for (int i=0;i<num_of_cpus;i++) {
        if (tile_table_get_pid_count(table, i) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Checks if there is a need to move pids from one cluster to another.
 * Calculates the average miss rate by cluster and compares every clusters
 * value to a limit specified by X*avg_miss_rate.
 * If the limit is met, migrate a pid to a new cluster and rearrange tile_table.
 */
void check_for_migration(cpu_set_t **clusters, tile_table_t *table) {

    int new_cluster;
    float total_miss_rate = 0;
    float avg_miss_rate;

    for (int i=0;i<NUM_OF_MEMORY_DOMAINS;i++) {
        total_miss_rate += tile_table_get_cluster_miss_rate(table, i);
    }
    avg_miss_rate = total_miss_rate / NUM_OF_MEMORY_DOMAINS;

    float limit = 1.5*avg_miss_rate;
    for (int i=0;i<NUM_OF_MEMORY_DOMAINS;i++) {
        if (tile_table_get_cluster_miss_rate(table, i) > limit &&
            tile_table_get_cluster_pid_count(table,i) > 1) {
        
            new_cluster = get_cluster_with_min_contention(table);
            pid_t pid_to_move = get_cluster_min_pid(table, i);
            if (tmc_cpus_set_my_affinity(clusters[new_cluster])) {
                tmc_task_die("sched_algs: Failure in tmc_cpus_set_my_affinity");
            }
            tile_table_move(table, pid_to_move, new_cluster);
        }
    }

}

/*void check_for_migration(tile_table_t *table, cpu_set_t *cpus_ptr, int tile_count) {
	int new_tile;
	float total_miss_rate;
    float avg_miss_rate;
    for (int i=0;i<tile_count;i++) {
        total_miss_rate += tile_table_get_miss_rate(table, i);
    }
    avg_miss_rate = total_miss_rate / tile_count;

	// 1.5 is a number I just made up
    float limit = 1.5*avg_miss_rate;
    for (int i=0;i<tile_count;i++) {
        if (tile_table_get_miss_rate(table, i) > limit && tile_table_get_pid_count(table, i) > 1) {
        	new_tile = get_tile(tile_count, table);
        	pid_t pid_to_move = tile_table_get_minimum_pid(table, i);
        	if (tmc_cpus_set_task_cpu((tmc_cpus_find_nth_cpu(cpus_ptr, new_tile)), pid_to_move) < 0) {
        		tmc_task_die("Failure in tmc_cpus_set_task_cpu (in migrate_process)");
        	}
        	tile_table_move(table, pid_to_move, new_tile);
        	//break;
        }
    }
}*/
