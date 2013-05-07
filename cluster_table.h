/* cluster_table.h
 *
 * Module to store cpu cluster data. The table has an index with with data for
 * each cpu cluster and a set to hold data about all running processes.
 * The data for each cpu cluster includes the number of processes running on
 * the cluster an contention values for each tile. */

#ifndef _CLUSTER_TABLE_H
#define _CLUSTER_TABLE_H

#include <unistd.h>
#include "pid_set.h"

/* Type definition of a cpu cluster table instance. */
typedef struct ctable_struct ctable_t;

/* Allocates a new cpu cluster table. On success a pointer handle to the table
 * is returned, otherwise NULL. */
ctable_t *ctable_create(void);

/* Destroys the specified cluster table. */
void ctable_destroy(ctable_t *table);

/* Adds the specified process ID with the specified cpu cluster allocation and
 * class to the table. On success 0 is returned, otherwise -1. */
int ctable_insert(ctable_t *table, pid_t pid, int cluster, int class);

/* Removes the specified process ID from the table. Returns 0 on success,
 * otherwise -1. */
int ctable_remove(ctable_t *table, pid_t pid);

pid_t ctable_get_minimum_pid(ctable_t *table, int cluster);

/* Moves the entry with the specified process ID to the specified cpu cluster.
 * Returns 0 on success, otherwise -1. */
int ctable_move_pid(ctable_t *table, pid_t pid, int cluster);

/* Returns the number of processes running on the specified cpu cluster or -1 if
 * an error occurred. */
int ctable_get_count(ctable_t *table, int cluster);

/* Updates the miss rate of the specified tile in the specified cpu cluster. On
 * success 0 is returned, otherwise -1. */
int ctable_set_miss_rate(ctable_t *table, int cluster, int index, float value);

/* Returns the miss rate of the specified tile in the specified cluster. On
 * success the miss rate value is returned, otherwise -1. */
float cable_get_miss_rate(ctable_t *table, int cluster, int index);

/**/
const float **ctable_get_miss_rates(ctable_t *table);

/* Returns the accumulated class value of all processes running on the specified
 * tile, or -1 on failure. */
int ctable_get_class_value(ctable_t *table, pid_t pid);

#endif /* _CLUSTER_TABLE_H */
