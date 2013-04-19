/* pid_table_test.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pid_table.h"

static const int index_size = 4;
static const int bucket_count = 1;
static const int num_entry = 8;
static const int num_cpu = 64;

// Performs a test of the pid_table module:
int main(int argc, char *argv[])
{
	int n, pid, cpu, new_cpu;
	pid_t pids[num_entry];
	pid_table table;

	// Create table:
	printf("creating table index: %i buckets: %i\n", index_size, bucket_count);
	table = create_pid_table(index_size, bucket_count);
	printf("table created\n");
	// Seed rand():
	srand(time(NULL ));
	// Add entries:
	printf("adding %i entries\n", num_entry);
	for (n = 0; n < num_entry; n++)
	{
		pid = rand();
		pids[n] = pid;
		cpu = rand() % num_cpu;
		add_pid(table, pid, cpu);
	}
	printf("adding complete\n");
	print_table(table);
	for (n = 0; n < num_entry; n++)
	{
		new_cpu = rand() % 64;
		printf("changing pid: %i cpu: %i -> %i\n", pids[n],
				get_cpu(table, pids[n]), new_cpu);
		set_cpu(table, pids[n], new_cpu);
	}
	print_table(table);
//	 Remove elements:
	printf("removing entries\n");
	for (n = 0; n < num_entry; n++)
	{
		remove_pid(table, pids[n]);
	}
	printf("remove complete\n");
	print_table(table);
	printf("destroying table\n");
	destroy_pid_table(table);
	printf("table destroyed\n");
	return 0;
}
