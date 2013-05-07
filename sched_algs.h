/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

#include "cluster_table.h"

int get_tile(int tile_count, ctable_t *table);

int get_cluster_with_min_contention(ctable_t *table);
int get_empty_cluster(ctable_t *table);

int get_tile_by_classes(int tile_count, ctable_t *table);
int get_tile_by_miss_rate(int tile_count, ctable_t *table);
int get_empty_tile(int tile_count, ctable_t *table);

void check_for_migration(cpu_set_t **clusters, ctable_t *table);
//void check_for_migration(ctable_t *table, cpu_set_t *cpus_ptr, int tile_count);
