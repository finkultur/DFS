/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

#include "tile_table.h"

int get_tile(cpu_set_t *cpus, tile_table_t *table);
int get_tile_by_classes(cpu_set_t *cpus, tile_table_t *table);
int get_tile_by_miss_rate(cpu_set_t *cpus, tile_table_t *table);
int get_empty_tile(int num_of_cpus, tile_table_t *table);
