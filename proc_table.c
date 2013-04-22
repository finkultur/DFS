/* proc_table.c
 * This is a combined interface to pid_table/tile_table
 * */

/*
 * How do I keep count of number of processes?
 * How do I...
 *
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include "pid_table.h"
#include "tile_table.h"
#include "proc_table.h"

// When do I need these?
#define MAX_PIDS_PER_TILE 32
#define MAX_PIDS_PER_INDEX 32

struct proc_table_struct {
	tile_table tile_table;
	pid_table pid_table;
};

// Are the calloc necessary here or just needed in pid_table / tile_table?
struct proc_table_struct *create_proc_table(size_t num_tiles) {
	struct proc_table_struct *table;

	if ((table = calloc(1, sizeof(struct proc_table_struct))) == NULL) {
		return NULL;
	}

	if ((table->tile_table = calloc(1, sizeof(struct tile_table_struct))) == NULL
	   || (table->pid_table = calloc(1, sizeof(struct pid_table_struct))) == NULL)
	{
		free(table);
		return NULL;
	}

    if (table->pid_table = create_pid_table(ADSADASD) == NULL) {
        return NULL;
    }
    if (table->tile_table = create_tile_table(ASDASDASDIUASD) == NULL) {
        return NULL;
    }

	return table;
}

// Do I need to do free something else here?
void destroy_proc_table(struct proc_table_struct *table) {
    destroy_pid_table(table->pid_table_struct);
    destroy_tile_Table(table->tile_table_struct);
}

int add_pid(proc_table table, pid_t pid, int tile_num) {
    if (add_pid_to_pid_table(table->pid_table_struct, pid, tile_num) != 0) {
        return -1;
    }
    if (add_pid_to_tile_table(table->tile_table_struct, pid, tile_num) != 0) {
        return -1;
    }
    return 0;
}

int remove_pid(proc_table table, pid_t pid) {
    if (remove_pid_from_pid_table(table->pid_table, pid) != 0) {
        return -1;
    }
    if (remove_pid_from_tile_table(table->tile_table, pid) != 0) {
        return -1;
    }
    return 0;
}

int move_pid_to_tile(proc_table table, pid_t pid, int new_tile_num) {
    if (remove_pid(table, pid) != 0) {
        return -1;
    }
    if (add_pid(table, pid, new_tile_num) != 0) {
        return -1;
    }
    return 0;
}

int get_pid_count(proc_table table, int tile_num) {

}

/*
 * 
 */
const pid_t *get_pid_vector(proc_table, int tile_num) {

}

