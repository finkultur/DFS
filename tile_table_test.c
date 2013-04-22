/* pid_table_test.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tile_table.h"

static const int num_tiles = 8;
static const int min_pids = 100;
static const int num_pids = 200;

// Performs a test of the pid_table module:
int main(int argc, char *argv[])
{
	int n, pid, cpu, new_cpu;
	pid_t pids[num_pids];
	tile_table table;

	// Create table:
	printf("creating pid_table index size: %i buckets: %i\n", num_tiles, min_pids);
	table = create_tile_table(num_tiles, min_pids);
	if (table == NULL)
	{
		printf("failed!\n");
		return 1;
	}
	printf("OK!\n");

	// Seed rand():
	srand(time(NULL ));
	// Add entries:
	printf("adding %i entries\n", num_pids);
	for (n = 0; n < num_pids; n++)
	{
		pid = rand();
		pids[n] = pid;
		cpu = rand() % num_tiles;
		if (add_pid(table, pid, cpu) != 0)
		{
			printf("failed!\n");
			return 1;
		}
	}
	printf("OK!\n");

	// Remove elements:
	printf("removing each entry\n");
	for (n = 0; n < num_pids; n++)
	{
		if (remove_pid(table, pids[n]) != 0)
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
