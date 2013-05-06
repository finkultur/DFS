/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

#include "tile_table.h"

int get_tile(int tile_count, tile_table_t *table);
int get_tile_by_classes(int tile_count, tile_table_t *table);
int get_tile_by_miss_rate(int tile_count, tile_table_t *table);
int get_empty_tile(int tile_count, tile_table_t *table);
void check_for_migration(tile_table_t *table, cpu_set_t *cpus_ptr, int tile_count);
