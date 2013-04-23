/* tile_table.c
 *
 * Implementation of the tile_table module.
 */

#include <stdlib.h>
#include "tile_table.h"

/* Each position in the table index represents a tile (CPU) and is implemented
 * with an index_struct. This data type holds the vector of process IDs running
 * on the tile. It also contains a count of the number of entries in the process
 * ID vector as well as its capacity. */
struct index_struct
{
	size_t entry_count;
	size_t bucket_count;
	pid_t *buckets;
};

/* A tile_table instance is represented by the tile_table_struct. This data type
 * contains the table index vector of tiles. It also holds the number of tiles
 * in the index vector as well as the minimum number of buckets allocated to
 * each index position. */
struct tile_table_struct
{
	size_t index_size;
	size_t min_buckets;
	struct index_struct *index;
};

// Function declarations, see below:
static int grow_bucket_vector(tile_table table, unsigned int cpu);
static int shrink_bucket_vector(tile_table table, unsigned int cpu);

// Create a new table:
tile_table create_tile_table(size_t num_cpu, size_t num_pid)
{
	int table_index;
	struct tile_table_struct *new_table;
	struct index_struct *new_index;
	pid_t *new_bucket;

	if (num_cpu == 0)
	{
		return NULL ;
	}
	// Allocate table, return on error:
	new_table = malloc(sizeof(struct tile_table_struct));
	if (new_table == NULL )
	{
		return NULL ;
	}
	// Allocate index, return on error:
	new_index = malloc(num_cpu * sizeof(struct index_struct));
	if (new_index == NULL )
	{
		return NULL ;
	}
	// Initialize table and index, allocate pids vectors. Return on error:
	new_table->index_size = num_cpu;
	new_table->min_buckets = num_pid;
	new_table->index = new_index;
	for (table_index = 0; table_index < num_cpu; table_index++)
	{
		new_bucket = malloc(num_pid * sizeof(pid_t));
		if (new_bucket == NULL )
		{
			return NULL ;
		}
		new_table->index[table_index].entry_count = 0;
		new_table->index[table_index].bucket_count = num_pid;
		new_table->index[table_index].buckets = new_bucket;
	}
	return new_table;
}

// Deallocate table and all entries:
void destroy_tile_table(tile_table table)
{
	int table_index;
	if (table == NULL )
	{
		return;
	}
	// Deallocate buckets:
	for (table_index = 0; table_index < table->index_size; table_index++)
	{
		free(table->index[table_index].buckets);
	}
	// Deallocate table index:
	free(table->index);
	// Deallocate table:
	free(table);
}

// Add pid to list of running processes on specified tile:
int add_pid_to_tile_table(tile_table table, pid_t pid, unsigned int cpu)
{
	if (table == NULL || cpu >= table->index_size
			|| grow_bucket_vector(table, cpu) != 0)
	{
		return -1;
	}
	// Add pid to tile, on last position in vector:
	table->index[cpu].buckets[table->index[cpu].entry_count] = pid;
	table->index[cpu].entry_count++;
	return 0;
}

// Remove pid from list of running processes on specified tile:
int remove_pid_from_tile_table(tile_table table, pid_t pid, unsigned int cpu)
{
	int bucket_index, shift_index;

	if (table == NULL || cpu >= table->index_size)
	{
		return -1;
	}
	// Find pid index in bucket vector:
	for (bucket_index = 0; bucket_index < table->index[cpu].entry_count;
			bucket_index++)
	{
		if (table->index[cpu].buckets[bucket_index] == pid)
		{
			break;
		}
	}
	// No pid found, return error:
	if (bucket_index == table->index[cpu].entry_count)
	{
		return -1;
	}
	// Remove entry from table:
	if (bucket_index != table->index[cpu].entry_count - 1)
	{
		shift_index = bucket_index;
		while (shift_index < table->index[cpu].entry_count - 1)
		{
			table->index[cpu].buckets[shift_index] =
					table->index[cpu].buckets[shift_index + 1];
			shift_index++;
		}
	}
	table->index[cpu].entry_count--;
	// Shrink bucket vector and return:
	return (shrink_bucket_vector(table, cpu));
}

// Get number of processes running on tile:
int get_pid_count_from_tile(tile_table table, unsigned int cpu)
{
	if (table == NULL || cpu >= table->index_size)
	{
		return -1;
	}
	return table->index[cpu].entry_count;
}

// Get list of all pids running on a given tile:
int get_pids(tile_table table, unsigned int cpu, pid_t *pids, size_t num_pid)
{
	int pid_index;

	if (table == NULL || cpu >= table->index_size)
	{
		return -1;
	}
	// Copy running pids to parameter array:
	for (pid_index = 0;
			pid_index < table->index[cpu].entry_count && pid_index < num_pid;
			pid_index++)
	{
		pids[pid_index] = table->index[cpu].buckets[pid_index];
	}
	return pid_index;
}

/* Doubles the size of the process ID vector at the specified tile if the
 * vector is full. */
static int grow_bucket_vector(tile_table table, unsigned int cpu)
{
	size_t new_bucket_count;
	pid_t *new_buckets;

	// Check if pid vector should be grown (full), otherwise return:
	if (table->index[cpu].entry_count < table->index[cpu].bucket_count)
	{
		return 0;
	}
	// Double bucket count:
	new_bucket_count = 2 * table->index[cpu].bucket_count;
	// Reallocate bucket vector:
	new_buckets = realloc(table->index[cpu].buckets,
			new_bucket_count * sizeof(pid_t));
	if (new_buckets == NULL )
	{
		return -1;
	}
	table->index[cpu].bucket_count = new_bucket_count;
	table->index[cpu].buckets = new_buckets;
	return 0;
}

/* Shrinks (halves) the bucket vector at the specified index until it is at
 * least 25% full. */
static int shrink_bucket_vector(tile_table table, unsigned int cpu)
{
	size_t new_bucket_count;
	pid_t *new_buckets;

	// Check if bucket vector size should be shrunk, otherwise return:
	if (table->index[cpu].bucket_count <= table->min_buckets
			|| table->index[cpu].entry_count
					>= (table->index[cpu].bucket_count / 4))
	{
		return 0;
	}
	// Calculate new bucket vector size:
	new_bucket_count = table->index[cpu].bucket_count;
	do
	{
		new_bucket_count = new_bucket_count / 2;
	} while (new_bucket_count > table->min_buckets
			&& (new_bucket_count / 4) > table->index[cpu].entry_count);
	// Reallocate bucket vector:
	new_buckets = realloc(table->index[cpu].buckets,
			new_bucket_count * sizeof(pid_t));
	if (new_buckets == NULL )
	{
		return -1;
	}
	table->index[cpu].bucket_count = new_bucket_count;
	table->index[cpu].buckets = new_buckets;
	return 0;
}
