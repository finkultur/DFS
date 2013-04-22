/* tile_table.c
 *
 * Implementation of the tile_table module.
 */

#include <stdlib.h>
#include "tile_table.h"

/* Each index is represented by an index_struct. This data type holds the
 * vector of data buckets, the number of available buckets and a count of the
 * number of entries stored at the index. */
struct tile_struct
{
	size_t pid_count;
	size_t bucket_count;
	pid_t *pid;
};

/* A tile_table instance is represented by the table_struct. This data type
 * contains the hash index vector and the size of the index. It also the
 * minimum number of buckets as specified on table creation. */
struct tile_table_struct
{
	size_t min_proc_space;
	size_t num_tiles;
	struct tile_struct *tile;
};

// Function declarations, see below:
static int grow_pid_vector(tile_table table, unsigned int cpu);
static int shrink_pid_vector(tile_table table, unsigned int cpu);

// Create a new table:
tile_table create_tile_table(size_t num_tiles, size_t min_proc_space)
{
	struct tile_table_struct *new_table;
	struct tile_struct *new_tiles;
	pid_t *pids;
	int i;

	if (num_tiles == 0)
	{
		return NULL ;
	}
	// Allocate table, return on error:
	new_table = malloc(sizeof(struct tile_table_struct));
	if (new_table == NULL )
	{
		return NULL ;
	}
	// Allocate tiles, return on error:
	new_tiles = malloc(num_tiles * sizeof(struct tile_struct));
	if (new_tiles == NULL )
	{
		return NULL ;
	}

	// Initialize table and index, allocate data buckets. Return on error:
	new_table->min_proc_space = min_proc_space;
	new_table->num_tiles = num_tiles;
	new_table->tile = new_tiles;
	// Allocate pid vector, return on error:
	for (i = 0; i < num_tiles; i++)
	{
		new_table->tile[i].pid_count = 0;
		pids = malloc(min_proc_space * sizeof(pid_t));
		if (pids == NULL )
		{
			return NULL ;
		}
		new_table->tile[i].pid = pids;
	}

	return new_table;
}

// Deallocate table and all entries:
void destroy_tile_table(tile_table table)
{
	if (table == NULL )
	{
		return;
	}

	// Deallocate tiles:
	free(table->tile);
	// Deallocate table:
	free(table);
}

// Add new pid to tile table:
int add_pid_to_tile_table(tile_table table, pid_t pid, unsigned int cpu)
{
	size_t bucket_count = table->tile[cpu].bucket_count;

	if (table == NULL )
	{
		return -1;
	}

	if (table->tile[cpu].bucket_count == table->tile[cpu].pid_count)
	{
		if (grow_pid_vector(table, cpu) != 0)
		{
			return -1;
		}
	}

	// Add entry pid to tile, last in the array:
	table->tile[cpu].pid[bucket_count] = pid;

	return 0;
}

// Remove pid from tile table:
int remove_pid_from_tile_table(tile_table table, pid_t pid, unsigned int cpu)
{
	int pid_index, shift_index;

	if (table == NULL )
	{
		return -1;
	}

	for(pid_index = 0; pid_index < table->tile[cpu].pid_count; pid_index++)
	{
		if (table->tile[cpu].pid[pid_index] == pid)
		{
			break;
		}
	}
	// No pid found, return error:
	if (pid_index == table->tile[cpu].pid_count)
	{
		return -1;
	}

	// Remove entry from table:
	if (pid_index != table->tile[cpu].pid_count - 1)
	{
		shift_index = pid_index;
		while (shift_index < table->tile[cpu].pid_count - 1)
		{
			table->tile[cpu].pid[shift_index] = table->tile[cpu].pid[shift_index
					+ 1];
			shift_index++;
		}
	}
	table->tile[cpu].pid_count--;
	// Shrink bucket vector and return:
	return (shrink_pid_vector(table, cpu));
}

// Get all pids on a given cpu.
void get_pids_on_tile(tile_table table, pid_t *return_pid, unsigned int cpu)
{
	return_pid = table->tile[cpu].pid;
}

/* Doubles the size of the bucket vector at the specified index if all buckets
 * are full. */
static int grow_pid_vector(struct tile_table_struct *table, unsigned int cpu)
{
	size_t new_vector_size;
	pid_t *new_pid_vector;
	int pid_vector_size = table->tile[cpu].bucket_count;

	// Check if pid vector should be grown (full), otherwise return:
	if (table->tile[cpu].pid_count < pid_vector_size)
	{
		return 0;
	}
	// Double bucket count:
	new_vector_size = 2 * pid_vector_size;
	// Reallocate bucket vector:
	new_pid_vector = realloc(table->tile[cpu].pid,
			new_vector_size * sizeof(pid_t));
	if (new_pid_vector == NULL )
	{
		return -1;
	}
	table->tile[cpu].pid = new_pid_vector;
	return 0;
}

/* Shrinks (halves) the bucket vector at the specified index until it is at
 * least 25% full. */
static int shrink_pid_vector(tile_table table, unsigned int cpu)
{
	size_t new_pid_vector_size;
	pid_t *new_pid_vector;
	int pid_vector_size = sizeof(table->tile[cpu].pid) / sizeof(pid_t);

	// Check if bucket vector size should be shrunk, otherwise return:
	if (pid_vector_size <= table->min_proc_space
			|| table->tile[cpu].pid_count >= (pid_vector_size / 4))
	{
		return 0;
	}
	// Calculated new pid vector:
	new_pid_vector_size = pid_vector_size;
	do
	{
		new_pid_vector_size = new_pid_vector_size / 2;
	} while (new_pid_vector_size > table->min_proc_space
			&& (new_pid_vector_size / 4) > table->tile[cpu].pid_count);
	// Reallocate bucket vector:
	new_pid_vector = realloc(table->tile[cpu].pid,
			new_pid_vector_size * sizeof(table->tile[cpu].pid));
	if (new_pid_vector == NULL )
	{
		return -1;
	}
	table->tile[cpu].pid = new_pid_vector;
	return 0;
}
