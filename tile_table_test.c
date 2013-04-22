/* pid_table_test.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tile_table.h"

static const int num_cpu = 4;
static const int num_pid = 2;
static const int num_entry = 1000;

// Performs a test of the tile_table module:
int main(int argc, char *argv[])
{
	int n, pid, cpu;
	int entry_cpu[num_entry];
	pid_t entry_pid[num_entry];
	tile_table table;

	// Create table:
	printf("creating table\n");
	table = create_tile_table(num_cpu, num_pid);
	if (table == NULL )
	{
		printf("failed!\n");
		return 1;
	}
	printf("OK!\n");
	// Seed rand():
	srand(time(NULL ));
	// Add entries:
	printf("adding %i entries\n", num_entry);
	for (n = 0; n < num_entry; n++)
	{
		pid = rand();
		cpu = rand() % num_cpu;
		entry_pid[n] = pid;
		entry_cpu[n] = cpu;
		if (add_pid(table, pid, cpu) != 0)
		{
			printf("failed!\n");
			return 1;
		}
	}
	printf("OK!\n");
	// Remove elements:
	printf("removing each entry\n");
	for (n = 0; n < num_entry; n++)
	{
		if (remove_pid(table, entry_pid[n], entry_cpu[n]) != 0)
		{
			printf("failed!\n");
			return 1;
		}
	}
	printf("OK!\n");
	// Destroy table
	printf("destroying table\n");
	destroy_tile_table(table);
	printf("OK!\n");
	return 0;
}
