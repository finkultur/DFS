/* tile_table.h
 *
 * Module to store process IDs of all running processes. The table has an index
 * of all tiles that is used to record the number of processes running on each
 * tile and the accumulated class value.
 * */

#ifndef _TILE_TABLE_H
#define _TILE_TABLE_H

#include <unistd.h>
#include "pid_set.h"

/* Type definition of a tile table instance. */
typedef struct tile_table_struct tile_table_t;

/* Allocates a new tile table with the specified number of tiles. On success a
 * pointer handle to the table is returned, otherwise NULL. */
tile_table_t* tile_table_create(size_t num_cpu);

/* Destroys the specified tile table instance. */
void tile_table_destroy(tile_table_t *table);

/* Adds the specified process ID with the specified tile allocation and class
 * to the set of running processes. On success 0 is returned, if an equal
 * entry already exist 1 is returned and if the operation fails -1 is
 * returned. */
int tile_table_insert(tile_table_t *table, pid_t pid, int cpu, int class);

/* Removes the specified process ID from the set of running processes. Returns
 * 0 on success, otherwise -1. */
int tile_table_remove(tile_table_t *table, pid_t pid);

/* Moves a pid to a new tile */
int tile_table_move(tile_table_t *table, pid_t pid, int new_tile); 

/* Returns the number of processes running on the specified tile or -1 if
 * an error occurred. */
int tile_table_get_pid_count(tile_table_t *table, int cpu);

/* Returns the accumulated class value of all processes running on the
 * specified tile, or -1 on failure. */
int tile_table_get_class_value(tile_table_t *table, int cpu);

/* Gets the process ID with the lowest class value of all processes running on
 * the specified tile. On success the process ID is returned, otherwise -1. */
pid_t tile_table_get_minimum_pid(tile_table_t *table, int cpu);

/* Gets the current miss rate of the specified tile. On success the miss rate
 * value is returned, otherwise -1. */
float tile_table_get_miss_rate(tile_table_t *table, int cpu);

/* Updates the miss rate of the specified tile. On success 0 is returned,
 * otherwise -1. */
int tile_table_set_miss_rate(tile_table_t *table, float miss_rate, int cpu);

#endif /* _TILE_TABLE_H */
