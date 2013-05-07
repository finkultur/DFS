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
#include "cluster_table.h"
#include "sched_algs.h"

#define NUM_OF_MEMORY_DOMAINS 4

/*
 * Checks tile table for an empty cluster or the cluster
 * with least miss rate.
 */
int get_cluster_with_min_contention(ctable_t *table)
{
	int empty_cluster = get_empty_cluster(table);
	if (empty_cluster >= 0) {
		return empty_cluster;
	}

	int best_cluster = 0;
	float min_val = calc_cluster_miss_rate(ctable_get_miss_rates(table), 0); 
	float new_val;
	for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		new_val = calc_cluster_miss_rate(ctable_get_miss_rates(table), i);
		if (new_val < min_val) {
			min_val = new_val;
			best_cluster = i;
		}
	}
	return best_cluster;
}

/*
 * Total miss rate per cluster
 */
float calc_cluster_miss_rate(const float **miss_rates, int cluster) 
{
    int init = (cluster % 2) * 4;
    float cluster_miss_rate = 0;
    for (int row = init; row <= (init + 4); row++) {
        for (int col = init; col <= (init + 4); col++) {
            cluster_miss_rate += miss_rates[col][row];
        }
    }
    return cluster_miss_rate;
}

/*
 * Total miss rate for ALL clusters
 */
float calc_total_miss_rate(const float **miss_rates) 
{
    float total_miss_rate = 0;
    for (int x=0;x<8;x++) {
        for (int y=0;y<8;y++) {
            total_miss_rate += miss_rates[x][y];
        }
    }
    return total_miss_rate;
}

/*
 * Returns the index of an empty cluster if there is one.
 * If no cluster is empty, return -1.
 */
int get_empty_cluster(ctable_t *table)
{
	for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		if (ctable_get_count(table, i) == 0) {
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
void check_for_migration(cpu_set_t **clusters, ctable_t *table)
{
	int new_cluster;
	float total_miss_rate;
	float avg_miss_rate;
    const float **miss_rates;
    float limit;

    miss_rates = ctable_get_miss_rates(table);    
    total_miss_rate = calc_total_miss_rate(miss_rates);
	avg_miss_rate = total_miss_rate / NUM_OF_MEMORY_DOMAINS;
	limit = 1.5 * avg_miss_rate;

	for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		if (calc_cluster_miss_rate(miss_rates, i) > limit && ctable_get_count(table, i) > 1) {

			new_cluster = get_cluster_with_min_contention(table);
			pid_t pid_to_move = ctable_get_minimum_pid(table, i);
			if (tmc_cpus_set_my_affinity(clusters[new_cluster])) {
				tmc_task_die("sched_algs: Failure in tmc_cpus_set_my_affinity");
			}
			ctable_move_pid(table, pid_to_move, new_cluster);
		}
	}

}
