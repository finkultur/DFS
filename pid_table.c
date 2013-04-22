/* pid_table.c
 *
 * Implementation of the pid_table module.
 */

#include <stdlib.h>
#include "pid_table.h"

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
struct index_struct
{
	size_t entry_count;
	size_t bucket_count;
	struct pid_struct *buckets;
};

/* A pid_table instance is represented by the table_struct. This data type
 * contains the hash index vector and the size of the index. It also the
 * minimum number of buckets as specified on table creation. */
struct pid_table_struct
{
	size_t index_size;
	size_t min_buckets;
	struct index_struct *index;
};

// Function declarations, see below:
static inline int hash_value(pid_table table, pid_t pid);
static int insert_entry(pid_table table, pid_t pid, unsigned int cpu);
static int remove_entry(pid_table table, pid_t pid);
static int grow_bucket_vector(pid_table table, int table_index);
static int shrink_bucket_vector(pid_table table, int table_index);

// Create a new table:
pid_table create_pid_table(size_t index_size, size_t bucket_count)
{
	int table_index;
	struct pid_table_struct *new_table;
	struct index_struct *new_index;
	struct pid_struct *new_buckets;

	if (index_size == 0 || bucket_count == 0)
	{
		return NULL ;
	}
	// Allocate table, return on error:
	new_table = malloc(sizeof(struct pid_table_struct));
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
	new_table->min_buckets = bucket_count;
	new_table->index = new_index;
	for (table_index = 0; table_index < index_size; table_index++)
	{
		new_table->index[table_index].bucket_count = bucket_count;
		new_table->index[table_index].entry_count = 0;
		new_buckets = malloc(bucket_count * sizeof(struct pid_struct));
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
	// Deallocate data bucket vectors:
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
int add_pid_to_pid_table(pid_table table, pid_t pid, unsigned int cpu)
{
	if (table == NULL )
	{
		return -1;
	}
	// Add entry to table:
	return insert_entry(table, pid, cpu);
}

// Remove pid from table:
int remove_pid_from_pid_table(pid_table table, pid_t pid)
{
	if (table == NULL )
	{
		return -1;
	}
	// Remove entry from table:
	return remove_entry(table, pid);
}

//Get tile number associated with specified pid:
int get_cpu_from_pid(pid_table table, pid_t pid)
{
	int table_index, bucket_index, low_limit, high_limit;

	if (table == NULL )
	{
		return -1;
	}
	// Get table index:
	table_index = hash_value(table, pid);
	// Find bucket index for entry (binary search):
	low_limit = 0;
	high_limit = ((int) table->index[table_index].entry_count) - 1;
	while (low_limit <= high_limit)
	{
		// Calculate next index to test:
		bucket_index = (low_limit + high_limit) / 2;
		if (table->index[table_index].buckets[bucket_index].pid < pid)
		{
			// Adjust lower bound when current entry is smaller:
			low_limit = bucket_index + 1;
		}
		else if (table->index[table_index].buckets[bucket_index].pid > pid)
		{
			// Adjust higher bound when current entry is bigger:
			high_limit = bucket_index - 1;
		}
		else
		{
			// Return when a matching entry is found:
			return table->index[table_index].buckets[bucket_index].cpu;
		}
	}
	// No matching entry found, indicate error:
	return -1;
}

// Set tile number for specified pid:
int set_cpu(pid_table table, pid_t pid, unsigned int cpu)
{
	int table_index, bucket_index, low_limit, high_limit;

	if (table == NULL )
	{
		return -1;
	}
	// Get table index:
	table_index = hash_value(table, pid);
	// Find bucket index for entry (binary search):
	low_limit = 0;
	high_limit = ((int) table->index[table_index].entry_count) - 1;
	while (low_limit <= high_limit)
	{
		// Calculate next index to test:
		bucket_index = (low_limit + high_limit) / 2;
		if (table->index[table_index].buckets[bucket_index].pid < pid)
		{
			// Adjust lower bound when current entry is smaller:
			low_limit = bucket_index + 1;
		}
		else if (table->index[table_index].buckets[bucket_index].pid > pid)
		{
			// Adjust higher bound when current entry is bigger:
			high_limit = bucket_index - 1;
		}
		else
		{
			// Return when a matching entry is found:
			table->index[table_index].buckets[bucket_index].cpu = cpu;
			return 0;
		}
	}
	// No matching entry found, indicate error:
	return -1;
}

/* Returns the hash value for the specified process ID. This value is simply
 * calculated as the modulo of the process ID and the table index size. */
static inline int hash_value(pid_table table, pid_t pid)
{
	return pid % table->index_size;
}

/* Inserts a new entry to table. Entries inserted are kept sorted according to
 * process ID. This allows fast binary searching of the data buckets. */
static int insert_entry(pid_table table, pid_t pid, unsigned int cpu)
{
	int table_index, bucket_index, shift_index, low_limit, high_limit;

	// Get table index:
	table_index = hash_value(table, pid);
	// Find bucket index for new entry (binary search):
	bucket_index = 0;
	low_limit = 0;
	high_limit = ((int) table->index[table_index].entry_count) - 1;
	while (low_limit <= high_limit)
	{
		// Calculate next index to test:
		bucket_index = (low_limit + high_limit) / 2;
		if (table->index[table_index].buckets[bucket_index].pid < pid)
		{
			// Move lower bound when existing entry is smaller:
			low_limit = bucket_index + 1;
		}
		else if (table->index[table_index].buckets[bucket_index].pid > pid)
		{
			// Move upper bound when existing entry is bigger:
			high_limit = bucket_index - 1;
		}
		else
		{
			// Break if existing entry is equal (should not happen...):
			break;
		}
	}
	// If bucket vector is not empty, index may need to be adjusted:
	if (table->index[table_index].entry_count != 0
			&& table->index[table_index].buckets[bucket_index].pid < pid)
	{
		bucket_index++;
	}
	// Grow bucket vector to ensure space:
	if (grow_bucket_vector(table, table_index) != 0)
	{
		return -1;
	}
	// Shift entries upwards if not inserting on last index in bucket vector:
	if (bucket_index < table->index[table_index].entry_count)
	{
		shift_index = table->index[table_index].entry_count;
		while (shift_index > bucket_index)
		{
			table->index[table_index].buckets[shift_index] =
					table->index[table_index].buckets[shift_index - 1];
			shift_index--;
		}
	}
	// Insert new entry:
	table->index[table_index].entry_count++;
	table->index[table_index].buckets[bucket_index].pid = pid;
	table->index[table_index].buckets[bucket_index].cpu = cpu;
	return 0;
}

/* Removes an entry with a matching process ID (only one such entry should ever
 * exist). Any entries following the removed entry are shifted one step in the
 * bucket vector to eliminate gaps. */
static int remove_entry(pid_table table, pid_t pid)
{
	int table_index, bucket_index, shift_index, low_limit, high_limit;

	// Get table index:
	table_index = hash_value(table, pid);
	// Find entry bucket index (binary search):
	low_limit = 0;
	high_limit = ((int) table->index[table_index].entry_count) - 1;
	while (low_limit <= high_limit)
	{
		// Calculate next index to test:
		bucket_index = (low_limit + high_limit) / 2;
		if (table->index[table_index].buckets[bucket_index].pid < pid)
		{
			// Adjust lower bound when current entry is smaller:
			low_limit = bucket_index + 1;
		}
		else if (table->index[table_index].buckets[bucket_index].pid > pid)
		{
			// Adjust higher bound when current entry is bigger:
			high_limit = bucket_index - 1;
		}
		else
		{
			// Break when a matching entry is found:
			break;
		}
	}
	// Return with error if no matching entry found:
	if (low_limit > high_limit)
	{
		return -1;
	}
	// Remove found entry:
	// If not last entry in bucket vector, shift subsequent entries:
	if (bucket_index != table->index[table_index].entry_count - 1)
	{
		shift_index = bucket_index;
		while (shift_index < table->index[table_index].entry_count - 1)
		{
			table->index[table_index].buckets[shift_index] =
					table->index[table_index].buckets[shift_index + 1];
			shift_index++;
		}
	}
	table->index[table_index].entry_count--;
	// Shrink bucket vector and return:
	return shrink_bucket_vector(table, table_index);
}

/* Doubles the size of the bucket vector at the specified index if all buckets
 * are full. */
static int grow_bucket_vector(struct pid_table_struct *table, int table_index)
{
	size_t new_bucket_count;
	struct pid_struct *new_buckets;

	// Check if bucket vector should be grown (full), otherwise return:
	if (table->index[table_index].entry_count
			< table->index[table_index].bucket_count)
	{
		return 0;
	}
	// Double bucket count:
	new_bucket_count = 2 * table->index[table_index].bucket_count;
	// Reallocate bucket vector:
	new_buckets = realloc(table->index[table_index].buckets,
			new_bucket_count * sizeof(struct pid_struct));
	if (new_buckets == NULL )
	{
		return -1;
	}
	table->index[table_index].bucket_count = new_bucket_count;
	table->index[table_index].buckets = new_buckets;
	return 0;
}

/* Shrinks (halves) the bucket vector at the specified index until it is at
 * least 25% full. */
static int shrink_bucket_vector(pid_table table, int table_index)
{
	size_t new_bucket_count;
	struct pid_struct *new_buckets;

	// Check if bucket vector size should be shrunk, otherwise return:
	if (table->index[table_index].bucket_count <= table->min_buckets
			|| table->index[table_index].entry_count
					>= (table->index[table_index].bucket_count / 4))
	{
		return 0;
	}
	// Calculated new bucket count:
	new_bucket_count = table->index[table_index].bucket_count;
	do
	{
		new_bucket_count = new_bucket_count / 2;
	} while (new_bucket_count > table->min_buckets
			&& (new_bucket_count / 4) > table->index[table_index].entry_count);
	// Reallocate bucket vector:
	new_buckets = realloc(table->index[table_index].buckets,
			new_bucket_count * sizeof(struct pid_struct));
	if (new_buckets == NULL )
	{
		return -1;
	}
	table->index[table_index].bucket_count = new_bucket_count;
	table->index[table_index].buckets = new_buckets;
	return 0;
}
