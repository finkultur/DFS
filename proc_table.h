/* proc_table.h
 *
 * */

#ifndef _PROC_TABLE_H
#define _PROC_TABLE_H

#include <unistd.h>
#include "tile_table.h"
#include "pid_table.h"

//struct proc_table_struct;
struct proc_table_struct {
    int num_tiles;
    tile_table tile_table;
    pid_table pid_table;

    float total_miss_rate;
    float avg_miss_rate;
    float *miss_counters;
};

typedef struct proc_table_struct *proc_table;

proc_table create_proc_table(size_t num_tiles);

void destroy_proc_table(proc_table table);

int add_pid(proc_table table, pid_t pid, int tile_num);

int remove_pid(proc_table table, pid_t pid);

int move_pid_to_tile(proc_table table, pid_t pid, int new_tile_num);

int get_pid_count(proc_table table, int tile_num);

int get_pid_vector(proc_table table, int tile_num, pid_t *array_of_pids, int num_pid);

int get_tile_num(proc_table table, pid_t pid);

void modify_miss_count(proc_table table, int tile_num, float amount);

#endif
