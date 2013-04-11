/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

int get_tile(cpu_set_t *cpus, int *tileAlloc, float *wr_miss_rates);
int get_empty_tile(cpu_set_t *cpus, int *tileAlloc);
int get_tile_with_min_write_miss_rate(cpu_set_t *cpus);
int get_tile_from_wr_miss_array(cpu_set_t *cpus, float *wr_miss_rates);
