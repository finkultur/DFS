/* tile_table.h
 *
 * A hash table module used for recording the processes running on each tile
 * (CPU core).
 * The table is index by tile number starting from zero. To improve the
 * runtime performance, especially targeting memory efficiency, the data
 * buckets on each position in the hash index are implemented with a vector that
 * dynamically changes its size (growing and shrinking) to fit the number of
 * entries. For best performance, it is therefore important to consider the
 * number of processes the system will handle and size the table accordingly.
 * */

#ifndef _TILE_TABLE_H
#define _TILE_TABLE_H

#include <unistd.h>

/* Each table instance is represented by a tile_table_struct. */
struct tile_table_struct;

/* Typedef for a user handle to a table instance (table pointer). */
typedef struct tile_table_struct *tile_table;

/* Allocates a new table with the specified number of tiles and initial space
 * for process IDs per tile (number of buckets).
 * On success a handle to the table is returned, otherwise NULL. */
tile_table create_tile_table(size_t num_cpu, size_t num_pid);

/* Destroys the given table by freeing up allocated memory. All table entries
 * are also freed during the operation. */
void destroy_tile_table(tile_table table);

/* Adds the specified process ID to the list of running processes on the
 * specified tile. On success 0 is returned, otherwise -1. */
int add_pid_to_tile_table(tile_table table, pid_t pid, unsigned int cpu);

/* Removes the specified process ID from the list of running processes on the
 * specified tile. Returns 0 on success, otherwise -1. */
int remove_pid_from_tile_table(tile_table table, pid_t pid, unsigned int cpu);

/* Returns the number of processes running of the specified tile or -1 if the
 * an error occurred. */
int get_pid_count_from_tile(tile_table table, unsigned int cpu);

/* Returns the process IDs of all processes running on the specified tile. The
 * IDs are copied to the user specified array, pids, with a maximum number of
 * IDs to copy specified by num_pids.
 * On success the number of process IDs copied is returned, otherwise -1. */
int get_pids(tile_table table, unsigned int cpu, pid_t *pids, size_t num_pids);

#endif /* _TILE_TABLE_H */
