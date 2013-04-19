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

struct tile_table_struct;

typedef struct tile_table_struct *tile_table;

tile_table create_tile_table(size_t num_cpus, size_t num_cpu_procs);

void destroy_tile_table(tile_table table);

int add_pid(tile_table table, pid_t pid, unsigned int cpu);

int remove_pid(tile_table table, pid_t pid);

int get_proc_count(tile_table table, unsigned int cpu);

int get_procs(tile_table table, unsigned int cpu, pid_t *pids, size_t max_pids);

#endif
