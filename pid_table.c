/* pid_table.c
 *
 * Implementation of the pid_table module.
 */

#include <stdio.h>
#include <stdlib.h>
#include "pid_table.h"

/* Each table entry is represented by an entry_struct. This data type records a
 * process ID and the number of the tile allocated to the process. */
struct entry_struct
{
	pid_t pid;
	unsigned int cpu;
};

/* Each index is represented by an index_struct. This data type holds the
 * vector of data buckets, the number of available buckets and a count of the
 * number of entries stored at the index. */
struct index_struct
{
	size_t entry_count;
	size_t bucket_count;
	struct entry_struct *buckets;
};

/* A pid_table instance is represented by the table_struct. This data type
 * contains the hash index vector and the size of the index. */
struct table_struct
{
	size_t index_size;
	size_t min_bucket_size;
	struct index_struct *index;
};

// Helper functions, see below:
static inline int hash_value(pid_table table, pid_t pid);
static int insert_entry(pid_table table, pid_t pid, unsigned int cpu);
static int remove_entry(pid_table table, pid_t pid);
static int grow_bucket_vector(pid_table table, int table_index);
static int shrink_bucket_vector(pid_table table, int table_index);

// Create a new table:
pid_table create_pid_table(size_t index_size, size_t bucket_count)
{
	int table_index;
	struct table_struct *new_table;
	struct index_struct *new_index;
	struct entry_struct *new_buckets;

	if (index_size == 0 || bucket_count == 0)
	{
		return NULL ;
	}
	// Allocate table, return on error:
	new_table = malloc(sizeof(struct table_struct));
	if (new_table == NULL )
	{
		return NULL ;
	}
	// Allocate hash index, return on error:
	new_index = malloc(index_size * sizeof(struct index_struct));
	if (new_index == NULL )
	{
		return NULL ;
	}
	// Initialize table and index, allocate data buckets. Return on error:
	new_table->index_size = index_size;
	new_table->min_bucket_size = bucket_count;
	new_table->index = new_index;
	for (table_index = 0; table_index < index_size; table_index++)
	{
		new_table->index[table_index].bucket_count = bucket_count;
		new_table->index[table_index].entry_count = 0;
		new_buckets = malloc(bucket_count * sizeof(struct entry_struct));
		if (new_buckets == NULL )
		{
			return NULL ;
		}
		new_table->index[table_index].buckets = new_buckets;
	}
	return new_table;
}

// Deallocate table and all entries:
void destroy_pid_table(pid_table table)
{
	int table_index;

	if (table == NULL )
	{
		return;
	}
	// Deallocate each data bucket:
	for (table_index = 0; table_index < table->index_size; table_index++)
	{
		free(table->index[table_index].buckets);
	}
	// Deallocate hash index:
	free(table->index);
	// Deallocate table:
	free(table);
}

// Add new pid to table:
int add_pid(pid_table table, pid_t pid, unsigned int cpu)
{
	if (table == NULL )
	{
		return -1;
	}
	// Add entry to table:
	return insert_entry(table, pid, cpu);
}

// Remove pid from table:
int remove_pid(pid_table table, pid_t pid)
{
	if (table == NULL )
	{
		return -1;
	}
	// Remove entry from table:
	return remove_entry(table, pid);
}

// Get cpu number for pid:
//int get_cpu(pid_table table, pid_t pid)
//{
//	int table_index, bucket_index;
//	size_t entry_count;
//
//	if (table == NULL )
//	{
//		return -1;
//	}
//
//	table_index = hash_value(table, pid);
//	entry_count = table->index[table_index].entry_count;
//	if (entry_count == 0)
//	{
//		return -1;
//	}
//	for (bucket_index = 0; bucket_index < entry_count; bucket_index++)
//	{
//		if (table->index[table_index].buckets[bucket_index].pid == pid)
//		{
//			return table->index[table_index].buckets[bucket_index].cpu;
//		}
//	}
//	return -1;
//}

// Set tile_num for specified pid:
//int set_tile_num(pid_table table, pid_t pid, int tile_num)
//{
//	int hash_index;
//	struct pid_entry_struct *entry_itr;
//	// Get hashed table index:
//	hash_index = hash_value(table, pid);
//	// Search list at table index for a matching entry:
//	entry_itr = table->data[hash_index];
//	while (entry_itr != NULL )
//	{
//		// Update tile_num and return if matching entry is found:
//		if (entry_itr->pid == pid)
//		{
//			entry_itr->tile_num = tile_num;
//			return 0;
//		}
//		entry_itr = entry_itr->next;
//	}
//// Pid not found:
//	return -1;
//}

// Returns the hash value for the specified pid:
static inline int hash_value(struct table_struct *table, pid_t pid)
{
	return pid % table->index_size;
}

// Insert new entry in table:
static int insert_entry(pid_table table, pid_t pid, unsigned int cpu)
{
	int table_index, position, left_limit, right_limit;
	pid_t position_pid, left_pid, right_pid;
	size_t entry_count, bucket_count;

	// Calculate table index and current number of entries:
	table_index = hash_value(table, pid);
	entry_count = table->index[table_index].entry_count;
	// Find bucket position for new entry (binary search):
	left_limit = 0;
	right_limit = ((int) entry_count) - 1;
	position = right_limit > 0 ? (left_limit + right_limit) / 2 : -1;
	while (left_limit <= right_limit)
	{
		position_pid = table->index[table_index].buckets[position].pid;
		if (position_pid == pid)
		{
			break;
		}
		if (position > left_limit && position < right_limit)
		{
			left_pid = table->index[table_index].buckets[position - 1].pid;
			right_pid = table->index[table_index].buckets[position + 1].pid;
			if (left_pid <= pid && right_pid >= pid)
			{
				break;
			}
		}
		if (position_pid < pid)
		{
			left_limit = position + 1;
		}
		else if (position_pid > pid)
		{
			right_limit = position - 1;
		}
		position = (left_limit + right_limit) / 2;
	}
	// If bucket vector was empty:
	if (position == -1)
	{
		table->index[table_index].entry_count++;
		table->index[table_index].buckets[0].pid = pid;
		table->index[table_index].buckets[0].cpu = cpu;
		return 0;
	}
	// Adjust bucket position:
	if (position == 0 && position_pid < pid)
	{
		position = 1;
	}
	else if (position == entry_count - 1 && position_pid < pid)
	{
		position = entry_count;
	}
	else if (position_pid < pid)
	{
		position = position + 1;
	}
	// Reallocate bucket vector if full:
	bucket_count = table->index[table_index].bucket_count;
	if (entry_count == bucket_count)
	{
		if (grow_bucket_vector(table, table_index) != 0)
		{
			return -1;
		}
	}
	// Move elements if necessary:
	if (position < entry_count - 1)
	{
		while (entry_count > position)
		{
			table->index[table_index].buckets[entry_count] =
					table->index[table_index].buckets[entry_count - 1];
			entry_count = entry_count - 1;
		}
	}
	table->index[table_index].entry_count++;
	table->index[table_index].buckets[position].pid = pid;
	table->index[table_index].buckets[position].cpu = cpu;
	return 0;
}

/* Removes the entry at the specified index and bucket. Any entries following
 * the removed entry are shifted down one step in the bucket vector. */
static int remove_entry(pid_table table, pid_t pid)
{
	int table_index, bucket_index;
	size_t bucket_count, entry_count;

	table_index = hash_value(table, pid);
	entry_count = table->index[table_index].entry_count;
	// Find bucket index (binary search):
	bucket_index = entry_count / 2;
	while (bucket_index >= 0 && bucket_index < entry_count)
	{
		// Index found or last index reached?
		if (table->index[table_index].buckets[bucket_index].pid == pid)
		{
			break;
		}
		else if (bucket_index == 0 || bucket_index == entry_count - 1)
		{
			return -1;
		}
		// Adjust index:
		if (table->index[table_index].buckets[bucket_index].pid < pid)
		{
			bucket_index = (bucket_index + entry_count) / 2;
		}
		else if (table->index[table_index].buckets[bucket_index].pid > pid)
		{
			bucket_index = bucket_index / 2;
		}
	}
	// Not last entry at index?
	if (bucket_index < entry_count - 1)
	{
		// Shift following entries down:
		while (bucket_index < entry_count - 1)
		{
			table->index[table_index].buckets[bucket_index] =
					table->index[table_index].buckets[bucket_index + 1];
			bucket_index++;
		}
	}
	// Adjust entry count:
	entry_count--;
	table->index[table_index].entry_count = entry_count;
	bucket_count = table->index[table_index].bucket_count;
	// Shrink bucket vector if less than 25% full:
	if ((bucket_count > table->min_bucket_size)
			&& (entry_count < bucket_count / 4))
	{
		if (shrink_bucket_vector(table, table_index) != 0)
		{
			return -1;
		}
	}
	return 0;
}

// Doubles the size of the bucket vector at the specified index:
static int grow_bucket_vector(struct table_struct *table, int table_index)
{
	size_t new_bucket_count;
	struct entry_struct *new_buckets;

	// Double bucket count:
	new_bucket_count = 2 * table->index[table_index].bucket_count;
	// Reallocate bucket vector:
	new_buckets = realloc(table->index[table_index].buckets,
			new_bucket_count * sizeof(struct entry_struct));
	if (new_buckets == NULL )
	{
		return -1;
	}
	table->index[table_index].bucket_count = new_bucket_count;
	table->index[table_index].buckets = new_buckets;
	return 0;
}

// Halves the bucket vector size until it is at least 25% full:
static int shrink_bucket_vector(pid_table table, int table_index)
{
	size_t new_bucket_count, entry_count;
	struct entry_struct *new_buckets;

	new_bucket_count = table->index[table_index].bucket_count;
	entry_count = table->index[table_index].entry_count;
	// Calculated new bucket count:
	do
	{
		new_bucket_count = new_bucket_count / 2;
	} while (new_bucket_count > table->min_bucket_size
			&& new_bucket_count / 4 > entry_count);
	// Reallocate bucket vector:
	new_buckets = realloc(table->index[table_index].buckets,
			new_bucket_count * sizeof(struct entry_struct));
	if (new_buckets == NULL )
	{
		return -1;
	}
	table->index[table_index].bucket_count = new_bucket_count;
	table->index[table_index].buckets = new_buckets;
	return 0;
}

// DEBUG
// Prints table to standard output:
void print_table(pid_table table)
{
	int table_index, bucket_index, entry_count, pid, cpu;

	printf("Printing table:\n");
	for (table_index = 0; table_index < table->index_size; table_index++)
	{
		entry_count = table->index[table_index].entry_count;
		if (entry_count == 0)
		{
			printf("index[%i]: empty\n", table_index);
		}
		else
		{
			printf("index[%i]: ", table_index);
			for (bucket_index = 0; bucket_index < entry_count; bucket_index++)
			{
				pid = table->index[table_index].buckets[bucket_index].pid;
				cpu = table->index[table_index].buckets[bucket_index].cpu;
				if (bucket_index < entry_count - 1)
				{
					printf("(pid: %i , cpu: %i) -> ", pid, cpu);
				}
				else
				{
					printf("(pid: %i , cpu: %i)\n", pid, cpu);
				}
			}
		}
	}
}
