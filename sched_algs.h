/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

int get_tile(cpu_set_t *cpus, int *tileAlloc);
int get_empty_tile(cpu_set_t *cpus, int *tileAlloc);
int get_tile_with_min_write_miss_rate(cpu_set_t *cpus);
