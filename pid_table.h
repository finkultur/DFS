/* pid_table.h
 *
 * A hash table module used to record process-to-tile (CPU core) allocation.
 * The table is indexed by a simple value calculated from the process ID modulo
 * the size of the hash index.
 * To improve the runtime execution performance, the data buckets on each index
 * are implemented with a dynamically growing vector. When creating a table, it
 * is therefore important to consider the initial size of both the hash index
 * and the data bucket vectors. These values should be properly adjusted to
 * ensure that the table will fit the expected number of elements in an
 * effective manner.
 */

#ifndef _PID_TABLE_H
#define _PID_TABLE_H

#include <unistd.h>

/* Each table instance is represented by a table_struct. */
struct table_struct;

/* Typedef for a user handle to a table instance (table pointer). */
typedef struct table_struct *pid_table;

/* Allocates a new table with the specified sizes for the hash index and the
 * data bucket vectors.
 * On success a handle to the table is returned, otherwise NULL. */
pid_table pt_create_table(size_t index_size, size_t bucket_count);

/* Destroys the given table by freeing up allocated memory. All table entries
 * are also freed during the operation. */
void pt_destroy_table(pid_table table);

/* Adds a new entry to the table with the specified process ID and allocated
 * tile. On success 0 is returned, otherwise -1. */
int pt_add_pid(pid_table table, pid_t pid, unsigned int cpu);

/* Removes the entry with the specified process ID from the table. Returns 0 on
 * success, otherwise -1. */
int pt_remove_pid(pid_table table, pid_t pid);

/* Tries to find an entry with the specified process ID and return the number
 * of the tile allocated to the process. On success the tile number is returned,
 * otherwise -1. */
int pt_get_cpu(pid_table table, pid_t pid);

/* Sets the tile number for the specified process ID to the specified value.
 * On success 0 is returned, otherwise -1. */
int pt_set_cpu(pid_table table, pid_t pid, unsigned int cpu);

#endif /* _PID_TABLE_H */
