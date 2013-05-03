/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

#include "proc_table.h"

int get_tile(cpu_set_t *cpus, proc_table table);
int get_tile_from_counters(cpu_set_t *cpus, proc_table table);
int get_tile_from_miss_rate(cpu_set_t *cpus, proc_table table, float *wr_miss_rates);
int get_empty_tile(int num_of_cpus, proc_table table);
int get_least_occupied_tile(int num_of_cpus, proc_table table);
int get_tile_with_min_write_miss_rate(cpu_set_t *cpus);
int get_tile_from_wr_miss_array(int num_of_cpus, float *wr_miss_rates);
