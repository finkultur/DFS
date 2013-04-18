/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

int get_tile(cpu_set_t *cpus, int *tileAlloc, float *wr_miss_rates);
int get_empty_tile(int num_of_cpus, int *tileAlloc);
int get_least_occupied_tile(int num_of_cpus, int *tileAlloc);
int get_tile_with_min_write_miss_rate(cpu_set_t *cpus);
int get_tile_from_wr_miss_array(int num_of_cpus, float *wr_miss_rates);
