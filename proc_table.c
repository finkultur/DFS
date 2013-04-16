/* proc_table.c
 *
 * */

#include <stdlib.h>

#define MAX_PIDS_PER_TILE 32

#define MAX_PIDS_PER_INDEX 32

/*
 * Contains all pids on a specific tile. num_pids is used for counting pids.
 * */
struct tile_entry_struct
{
	int num_pids;
	pid_t pid_vector[MAX_PIDS_PER_TILE];
};

/*
 * Contains an array of all tiles. num_tiles is used for counting tiles.
 * */
struct tile_table_struct
{
	size_t num_tiles;
	struct tile_entry_struct *tile_vector;
};

/*
 * Contains pid and tile number, for associating a pid with a tile number.
 * */
struct pid_entry_struct
{
	pid_t pid;
	int tile_num;
};

/*
 * Contains an array of touples of pids and tile numbers.
 * */
struct pid_index_struct
{
	int num_pids;
	struct pid_entry_struct pid_vector[MAX_PIDS_PER_INDEX];
};

/*
 *
 * */
struct pid_table_struct
{
	int num_indices;
	struct pid_index_struct *index_vector;
};

struct proc_table_struct
{
	struct tile_table_struct *tile_table;
	struct pid_table_struct *pid_table;
};

struct proc_table_struct *create_proc_table(size_t num_tiles)
{
	struct proc_table_struct *table;

	if ((table = calloc(1, sizeof(struct proc_table_struct))) == NULL )
	{
		return NULL ;
	}

	if ((table->tile_table = calloc(1, sizeof(struct tile_table_struct)))
			== NULL
			|| (table->pid_table = calloc(1, sizeof(struct pid_table_struct)))
					== NULL )
	{
		free(table);
		return NULL ;
	}

	if ((table->tile_table->tile_vector = calloc(num_tiles,
			sizeof(struct tile_entry_struct))) == NULL
			|| (table->pid_table->index_vector = calloc(num_tiles,
					sizeof(struct pid_index_struct))) == NULL )
	{
		free(table->tile_table);
		free(table->pid_table);
		free(table);
		return NULL ;
	}

	table->tile_table->num_tiles = num_tiles;
	table->pid_table->num_indices = num_tiles;

	return table;
}

void destroy_proc_table(struct proc_table_struct *table)
{
	free(table->tile_table->tile_vector);

	free(table->pid_table->index_vector);

	free(table->tile_table);

	free(table->pid_table);

	free(table);
}

int add_pid(struct proc_table_struct *table, pid_t pid, int tile_num)
{
	int tile_vector_index, pid_table_index, pid_vector_index;

	tile_vector_index = table->tile_table->tile_vector[tile_num].num_pids;

	pid_table_index = pid % table->pid_table->num_indices;
	pid_vector_index = table->pid_table->index_vector[pid_table_index].num_pids;

	if (tile_vector_index >= MAX_PIDS_PER_TILE
			|| pid_vector_index >= MAX_PIDS_PER_INDEX)
	{
		return 1;
	}

	table->tile_table->tile_vector[tile_num].pid_vector[tile_vector_index] =
			pid;
	table->tile_table->tile_vector[tile_num].num_pids++;

	table->pid_table->index_vector[pid_table_index].pid_vector[pid_vector_index].pid =
			pid;
	table->pid_table->index_vector[pid_table_index].pid_vector[pid_vector_index].tile_num =
			tile_num;
	return 0;
}

