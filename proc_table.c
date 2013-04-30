/* proc_table.c
 * This is a combined interface to pid_table/tile_table
 * */

#include <stdlib.h>
#include "pid_table.h"
#include "tile_table.h"
#include "proc_table.h"

// When do I need these?
#define START_PIDS_PER_TILE 32
#define START_PIDS_PER_INDEX 32
#define PID_TABLE_INDEX_SIZE 64

proc_table create_proc_table(size_t num_tiles) {
	proc_table table;

	if ((table = malloc(sizeof(struct proc_table_struct))) == NULL) {
		return NULL;
	}
    if ((table->pid_table = create_pid_table(PID_TABLE_INDEX_SIZE, START_PIDS_PER_INDEX)) == NULL) {
        return NULL;
    }
    if ((table->tile_table = create_tile_table(num_tiles, START_PIDS_PER_TILE)) == NULL) {
        return NULL;
    }
    if ((table->miss_counters = malloc(sizeof(int)*num_tiles)) == NULL) {
        return NULL;
    }
    table->num_tiles = num_tiles;
    for (int i=0;i<num_tiles;i++) {
        table->miss_counters[i] = 0;
    }
    table->total_miss_rate = 0;
    table->avg_miss_rate = 0.0;
	return table;
}

void destroy_proc_table(proc_table table) {
    destroy_pid_table(table->pid_table);
    destroy_tile_table(table->tile_table);
    free(table);
}

int add_pid(proc_table table, pid_t pid, int tile_num, int class) {
    if (add_pid_to_pid_table(table->pid_table, pid, tile_num, class) != 0) {
        return -1;
    }
    if (add_pid_to_tile_table(table->tile_table, pid, tile_num) != 0) {
        return -1;
    }
    return 0;
}

int remove_pid(proc_table table, pid_t pid) {
    int cpu = get_cpu(table->pid_table, pid);

    if (remove_pid_from_pid_table(table->pid_table, pid) != 0) {
        return -1;
    }
    if (remove_pid_from_tile_table(table->tile_table, pid, cpu) != 0) {
        return -1;
    }
    return 0;
}

int move_pid_to_tile(proc_table table, pid_t pid, int new_tile_num) {
    int old_cpu = get_cpu(table->pid_table, pid);

    // Fix pid_table
    if (set_cpu(table->pid_table, pid, new_tile_num) != 0) {
        return -1;
    }
    // Fix tile_table
    if (remove_pid_from_tile_table(table->tile_table, pid, old_cpu) != 0) {
        return -1;
    }
    if (add_pid_to_tile_table(table->tile_table, pid, new_tile_num) != 0) {
        return -1;
    }
    return 0;
}

int get_pid_count(proc_table table, int tile_num) {
    return get_pid_count_from_tile(table->tile_table, tile_num);
}

int get_pid_vector(proc_table table, int tile_num, pid_t *array_of_pids, int num_pids) {
    return get_pids(table->tile_table, tile_num, array_of_pids, num_pids);
}

int get_tile_num(proc_table table, pid_t pid) {
    return get_cpu(table->pid_table, pid);
}

int get_class(proc_table table, pid_t pid) {
	return get_class_number(table->pid_table, pid);
}

void modify_miss_count(proc_table table, int tile_num, float new_miss_rate) {
	// Delete old miss rate from total
    table->total_miss_rate = table->total_miss_rate - table->miss_counters[tile_num];
    // Calculate average miss rate for tile_num
    table->miss_counters[tile_num] = (table->miss_counters[tile_num] + new_miss_rate) / 2;
    // Add new mis rate to total
    table->total_miss_rate = table->total_miss_rate + table->miss_counters[tile_num];
    // Calculate average miss rate among all tiles
    table->avg_miss_rate = table->total_miss_rate / table->num_tiles;
}
