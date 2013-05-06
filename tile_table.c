/* tile_table.c
 *
 * Implementation of the tile table module.
 */

#include <stdlib.h>
#include "tile_table.h"

/* Type definition for a tile index entry. */
typedef struct tile_struct tile_t;

/* Data type for a tile index entry. */
struct tile_struct {
	size_t pid_count;
	int class_value;
	float miss_rate;
};

/* Data type for a tile table instance. */
struct tile_table_struct {
	size_t num_cpu;
	tile_t *cpu_index;
	pid_set_t *pid_set;
};

/* Creates a new table. */
tile_table_t *tile_table_create(size_t num_cpu)
{
	int i;
	tile_table_t *table;
	/* Check for valid index size. */
	if (num_cpu == 0) {
		return NULL;
	}
	/* Allocate table, index and table index pid sets. */
	if ((table = malloc(sizeof(tile_table_t))) == NULL) {
		return NULL;
	} else if ((table->cpu_index = malloc(num_cpu * sizeof(tile_t))) == NULL) {
		free(table);
		return NULL;
	} else if ((table->pid_set = pid_set_create()) == NULL) {
		free(table);
		return NULL;
	} else {
		table->num_cpu = num_cpu;
		for (i = 0; i < num_cpu; i++) {
			table->cpu_index[i].pid_count = 0;
			table->cpu_index[i].class_value = 0;
			table->cpu_index[i].miss_rate = 0.0f;
		}
		return table;
	}
}

/* Deallocate table and all entries. */
void tile_table_destroy(tile_table_t *table)
{
	/* Free index and set. */
	pid_set_destroy(table->pid_set);
	free(table->cpu_index);
	free(table);
}

/* Adds the process ID to the set of running processes. */
int tile_table_insert(tile_table_t *table, pid_t pid, int cpu, int class)
{
	int status;
	if (cpu < 0 || cpu >= table->num_cpu) {
		return -1;
	}
	status = pid_set_insert(table->pid_set, pid, cpu, class);
	if (status == 0) {
		table->cpu_index[cpu].pid_count++;
		table->cpu_index[cpu].class_value += class;
	}
	return status;
}

/* Removes the process ID from the set of running processes. */
int tile_table_remove(tile_table_t *table, pid_t pid)
{
	int cpu = pid_set_get_cpu(table->pid_set, pid);
	int class = pid_set_get_class(table->pid_set, pid);
	if (cpu == -1 || class == -1) {
		return -1;
	} else {
		pid_set_remove(table->pid_set, pid);
		table->cpu_index[cpu].pid_count--;
		table->cpu_index[cpu].class_value -= class;
		return 0;
	}
}

/* Returns the number of processes running on the specified tile. */
int tile_table_get_pid_count(tile_table_t *table, int cpu)
{
	if (cpu < 0 || cpu >= table->num_cpu) {
		return -1;
	} else {
		return table->cpu_index[cpu].pid_count;
	}
}

/* Returns the accumulated class value of all processes running on specified
 * tile. */
int tile_table_get_class_value(tile_table_t *table, int cpu)
{
	if (cpu < 0 || cpu >= table->num_cpu) {
		return -1;
	} else {
		return table->cpu_index[cpu].class_value;
	}
}

/* Returns the process ID with the lowest class value of all processes running
 * on the specified tile. */
pid_t tile_table_get_minimum_pid(tile_table_t *table, int cpu)
{
	if (cpu < 0 || cpu >= table->num_cpu) {
		return -1;
	} else {
		return pid_set_get_minimum_pid(table->pid_set, cpu);
	}
}

/* Returns the miss rate on the specified tile. */
float tile_table_get_miss_rate(tile_table_t *table, int cpu)
{
	if (cpu < 0 || cpu >= table->num_cpu) {
		return -1.0f;
	} else {
		return table->cpu_index[cpu].miss_rate;
	}
}

/* Updates the miss rate on the specified tile. */
int tile_table_set_miss_rate(tile_table_t *table, float miss_rate, int cpu)
{
	if (cpu < 0 || cpu >= table->num_cpu) {
		return -1;
	} else {
		table->cpu_index[cpu].miss_rate = miss_rate;
		return 0;
	}
}
