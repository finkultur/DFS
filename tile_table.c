/* tile_table.c
 *
 * Implementation of the tile_table module.
 */

#include <stdlib.h>
#include "tile_table.h"

/* Each table entry is represented by an pid_struct. This data type records a
 * process ID and the number of the tile allocated to the process. */
struct pid_struct
{
	pid_t pid;
	unsigned int cpu;
};

/* Each index is represented by an index_struct. This data type holds the
 * vector of data buckets, the number of available buckets and a count of the
 * number of entries stored at the index. */
struct tile_struct
{
	size_t pid_count;
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
static int
insert_entry(tile_table table, pid_t pid, unsigned int cpu);
static int
remove_entry(tile_table table, pid_t pid);
int find_index(tile_table table, pid_t pid, unsigned int cpu);
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
int add_pid(tile_table table, pid_t pid, unsigned int cpu)
{
	if (table == NULL )
	{
		return -1;
	}
	// Add entry to table:
	return insert_entry(table, pid, cpu);
}

// Remove pid from tile table:
int remove_pid(tile_table table, pid_t pid)
{
	if (table == NULL )
	{
		return -1;
	}
	// Remove entry from table:
	return remove_entry(table, pid);
}

// Set tile number for specified pid:
int set_cpu(tile_table table, pid_t pid, unsigned int cpu)
{
	int low_limit, high_limit, pid_index;

	if (table == NULL )
	{
		return -1;
	}

	// Find bucket index for entry (binary search):
	low_limit = 0;
	high_limit = (int) table->num_tiles - 1;
	while (low_limit <= high_limit)
	{
		// Calculate next index to test:
		pid_index = (low_limit + high_limit) / 2;
		if (table->tile[cpu].pid[pid_index] < pid)
		{
			// Adjust lower bound when current entry is smaller:
			low_limit = pid_index;
		}
		else if (table->tile[cpu].pid[pid_index] > pid)
		{
			// Adjust higher bound when current entry is bigger:
			high_limit = pid_index;
		}
		else
		{
			// Return when a matching entry is found:
			table->tile[cpu].pid[pid_index] = cpu;
			return 0;
		}
	}
	// No matching entry found, indicate error:
	return -1;
}

// Get all pids on a given cpu.
pid_t* get_cpu(tile_table table, unsigned int cpu)
{
	return table->tile[cpu].pid;
}

/* Inserts a new entry to table. Entries inserted are kept sorted according to
 * process ID. This allows fast binary searching of the data buckets. */
static int insert_entry(tile_table table, pid_t pid, unsigned int cpu)
{
	int pid_index, shift_index;

	// Find bucket index for new entry (binary search):
	pid_index = find_index(table, pid, cpu);

	// Grow bucket vector to ensure space:
	if (grow_pid_vector(table, cpu) != 0)
	{
		return -1;
	}
	// Shift entries upwards if not inserting on last index in bucket vector:
	if (pid_index < table->tile[cpu].pid_count)
	{
		shift_index = table->tile[cpu].pid_count;
		while (shift_index > pid_index)
		{
			table->tile[cpu].pid[shift_index] = table->tile[cpu].pid[shift_index
					- 1];
			shift_index--;
		}
	}
	// Insert new entry and increase counter:
	table->tile[cpu].pid_count++;
	table->tile[cpu].pid[pid_index] = pid;

	return 0;
}

/* Removes an entry with a matching process ID (only one such entry should ever
 * exist). Any entries following the removed entry are shifted one step in the
 * bucket vector to eliminate gaps. */
static int remove_entry(tile_table table, pid_t pid)
{
	int pid_index, cpu, shift_index;

	//Want to get the cpu of a gien pid. Either fetch or give as a parameter.
	//cpu = get_cpu(table, pid);
	pid_index = find_index(table, pid, cpu);

	// Remove found entry:
	// If not last entry in bucket vector, shift subsequent entries:
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
	table->tile[cpu].pid--;
	// Shrink bucket vector and return:
	return (shrink_pid_vector(table, table->num_tiles));
}

/* Uses binary search to find an index in the table. Either for insertion or deletion.
 *
 */
int find_index(tile_table table, pid_t pid, unsigned int cpu)
{
	int pid_index, low_limit, high_limit;

	// Find bucket index for new entry (binary search):
	pid_index = 0;
	low_limit = 0;
	high_limit = ((int) table->tile[cpu].pid_count) - 1;
	while (low_limit <= high_limit)
	{
		// Calculate next index to test:
		pid_index = (low_limit + high_limit) / 2;
		if (table->tile[cpu].pid[pid_index] < pid)
		{
			// Move lower bound when existing entry is smaller:
			low_limit = pid_index + 1;
		}
		else if (table->tile[cpu].pid[pid_index] > pid)
		{
			// Move upper bound when existing entry is bigger:
			high_limit = pid_index - 1;
		}
		else
		{
			// Break if existing entry is equal (should not happen...):
			break;
		}
	}
	// If bucket vector is not empty, index may need to be adjusted:
	if (table->tile[cpu].pid_count != 0
			&& table->tile[cpu].pid[pid_index] < pid)
	{
		pid_index++;
	}
	else if (low_limit > high_limit)
	{
		return -1;
	}

	return pid_index;
}

/* Doubles the size of the bucket vector at the specified index if all buckets
 * are full. */
static int grow_pid_vector(struct tile_table_struct *table, unsigned int cpu)
{
	size_t new_vector_size;
	pid_t *new_pid_vector;
	int pid_vector_size = sizeof(table->tile[cpu].pid) / sizeof(pid_t);

	// Check if pid vector should be grown (full), otherwise return:
	if (table->tile[cpu].pid_count < pid_vector_size)
	{
		return 0;
	}
	printf("Growth of vector imminent\n", NULL);
	printf("Current vector size: %i\n", pid_vector_size);
	// Double bucket count:
	new_vector_size = 2 * pid_vector_size;
	printf("New vector size: %i\n", new_vector_size);
	// Reallocate bucket vector:
	new_pid_vector = realloc(table->tile[cpu].pid,
			new_vector_size * sizeof(size_t));
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
