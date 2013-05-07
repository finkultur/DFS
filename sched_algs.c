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
	/*
	int best_cluster = 0;
	int min_val = ctable_get_miss_rate(table, 0);
	int new_val;
	for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		new_val = tile_table_get_cluster_miss_rate(i);
		if (new_val < min_val) {
			min_val = new_val;
			best_cluster = i;
		}
	}
	*/
	//return best_cluster;
	return 0;
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
	float total_miss_rate = 0;
	float avg_miss_rate;

	/*for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		total_miss_rate += tile_table_get_cluster_miss_rate(table, i);
	}*/
	avg_miss_rate = total_miss_rate / NUM_OF_MEMORY_DOMAINS;

	float limit = 1.5 * avg_miss_rate;
	for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		/*if (ctable_get_cluster_miss_rate(table, i) > limit
				&& tile_table_get_cluster_pid_count(table, i) > 1) {

			new_cluster = get_cluster_with_min_contention(table);
			pid_t pid_to_move = get_cluster_min_pid(table, i);
			if (tmc_cpus_set_my_affinity(clusters[new_cluster])) {
				tmc_task_die("sched_algs: Failure in tmc_cpus_set_my_affinity");
			}
			tile_table_move(table, pid_to_move, new_cluster);
		}*/
	}

}
