/* proc_table.h
 *
 * */

#ifndef _PROC_TABLE_H
#define _PROC_TABLE_H

#include <unistd.h>

struct proc_table_struct;

typedef struct proc_table_struct *proc_table;

proc_table create_proc_table(size_t num_tiles);

void destroy_proc_table(proc_table table);

int add_pid(proc_table table, pid_t pid, int tile_num);

int remove_pid(proc_table table, pid_t pid);

int move_pid_to_tile(proc_table table, pid_t pid, int new_tile_num);

int get_pid_count(proc_table table, int tile_num);

int get_pid_vector(proc_table table, int tile_num, pid_t *array_of_pids, int num_pid);

#endif
