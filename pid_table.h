/* pid_table.h
 *
 * A simple hash-table like module that is used to store data about process-id
 * to tile (cpu core) allocation.
 */

#ifndef _PID_TABLE_H
#define _PID_TABLE_H

#include <unistd.h>
#include <sys/types.h>

/* Each table instance is represented by a pid_table_struct. */
struct pid_table_struct;

/* Each table entry is represented by a pid_entry_struct. This struct holds
 * data about the process-id and the number of the tile allocated to the
 * process. */
struct pid_entry_struct;

/* Typedef for a table pointer. */
typedef struct pid_table_struct *pid_table;

/* Typedef for a table entry pointer. */
typedef struct pid_entry_struct *pid_entry;

/* Creates a new table of the desired size and returns a pointer to it. On
 * failure a NULL-pointer is returned. */
pid_table create_table(int size);

/* Destroys the given table by freeing up allocated memory. All table entries
 * are also freed during the operation. */
void destroy_table(pid_table table);

/* Get the size of the specified table. */
int get_size(pid_table table);

/* Get the current number of entries in the specified table. */
int get_count(pid_table table);

/* Adds a new entry to the table with the specified process-id and allocated
 * tile. On success 0 is returned, otherwise -1. */
int add_pid(pid_table table, pid_t pid, int tile_num);

/* Removes the entry with the specified pid from the table. Returns 0 on
 * success, otherwise -1. */
int remove_pid(pid_table table, pid_t pid);

/* Tries to find an entry with the specified pid and return the number of the
 * tile allocated to the process. On success the tile number is returned,
 * otherwise -1. */
int get_tile_num(pid_table table, pid_t pid);

/* Sets the tile number for the specified process-id to the specified value.
 * On success 0 is returned, otherwise -1. */
int set_tile_num(pid_table table, pid_t pid, int tile_num);

// FOR TESTING:
void print_table(pid_table table);

#endif /* _PID_TABLE_H */
