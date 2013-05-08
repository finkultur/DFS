/* cluster_table.c
 *
 * Implementation of the cpu cluster table.
 */
#include <stdlib.h>
#include "pid_set.h"
#include "cluster_table.h"

/* Number of tile columns. */
#define CPU_COLUMNS 8

/* Number of tile rows. */
#define CPU_ROWS 8

/* Number of cpu clusters. */
#define CPU_CLUSTERS 4

/* Data type representing a cpu cluster table. */
struct cluster_table_struct {
	pid_set_t *pid_set;
	int pid_counters[CPU_CLUSTERS];
	float cluster_miss_rates[CPU_CLUSTERS];
	float tile_miss_rates[CPU_COLUMNS][CPU_ROWS];
};

/* Creates a new cluster table. */
ctable_t *ctable_create(void)
{
	ctable_t *table = calloc(1, sizeof(ctable_t));
	pid_set_t *set = pid_set_create();
	if (table != NULL && set != NULL) {
		table->pid_set = set;
		return table;
	} else {
		pid_set_destroy(set);
		free(table);
		return NULL;
	}
}

/* Deallocates the specified table. */
void ctable_destroy(ctable_t *table)
{
	pid_set_destroy(table->pid_set);
	free(table);
}

/* Adds the specified process to the table. */
int ctable_insert(ctable_t *table, pid_t pid, int cluster, int class)
{
	if (cluster < 0 || cluster >= CPU_CLUSTERS
			|| pid_set_insert(table->pid_set,pid, cluster, class) != 0) {
		return -1;
	} else {
		table->pid_counters[cluster]++;
		return 0;
	}
}

/* Removes the specified process ID from the table. */
int ctable_remove(ctable_t *table, pid_t pid)
{
	int cluster = pid_set_get_cluster(table->pid_set, pid);
	if (cluster == -1) {
		return -1;
	} else {
		table->pid_counters[cluster]--;
		pid_set_remove(table->pid_set, pid);
		return 0;
	}
}

pid_t ctable_get_minimum_pid(ctable_t *table, int cluster)
{
	return pid_set_get_minimum_pid(table->pid_set, cluster);
}


int ctable_migrate(ctable_t *table, pid_t pid, int new_cluster)
{
	int old_cluster = pid_set_get_cluster(table->pid_set, pid);
	if (old_cluster < 0) {
		return -1;
	}
	pid_set_set_cluster(table->pid_set, pid, new_cluster);
	table->pid_counters[old_cluster]--;
	table->pid_counters[new_cluster]++;

	return 0;
}

/* Returns the number of running processes in the specified cluster. */
int ctable_get_count(ctable_t *table, int cluster)
{
	if (cluster < 0 || cluster >= CPU_CLUSTERS) {
		return -1;
	} else {
		return table->pid_counters[cluster];
	}
}


/**/
float **ctable_get_miss_rates(ctable_t *table)
{
	return (float**)table->tile_miss_rates;
}

int ctable_set_cluster_miss_rate(ctable_t *table, int cluster, float value)
{
	table->cluster_miss_rates[cluster] = value;
	return 0;
}

