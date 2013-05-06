/* pid_table_test.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tile_table.h"

// Performs a test of the tile_table module:
int main(int argc, char *argv[])
{
	int n, num_cpu, num_entry, pid, cpu, class, status;
	pid_t *entries;
	tile_table_t *table;
	// Check arguments:
	if (argc != 3) {
		printf("usage: %s <number of tiles> <number of entries>\n", argv[0]);
		return 1;
	}
	num_cpu = atoi(argv[1]);
	num_entry = atoi(argv[2]);
	if (num_cpu <= 0 || num_entry <= 0) {
		printf("invalid arguments\n");
		return 1;
	} else if ((entries = malloc(num_entry * sizeof(pid_t))) == NULL) {
		printf("allocation failure\n");
		return 1;
	}
	// Create table:
	printf("creating table\n");
	table = tile_table_create(num_cpu);
	if (table == NULL) {
		printf("failed!\n");
		return 1;
	}
	printf("OK!\n");
	srand(time(NULL));
	// Add entries:
	printf("adding %i entries\n", num_entry);
	for (n = 0; n < num_entry; n++) {
		pid = rand();
		cpu = rand() % num_cpu;
		class = rand() % 4;
		entries[n] = pid;
		status = tile_table_insert(table, pid, cpu, class);
		if (status == -1) {
			printf("insert failure!\n");
			return 1;
		} else if (status == 1) {
			printf("duplicate entry\n");
			entries[n] = -1;
		}
	}
	printf("OK!\n");
	// Remove elements:
	printf("removing each entry\n");
	for (n = 0; n < num_entry; n++) {
		if (entries[n] == -1) {
			continue;
		} else if (tile_table_remove(table, entries[n]) != 0) {
			printf("remove failed!\n");
			return 1;
		}
	}
	printf("OK!\n");
	// Destroy table
	printf("destroying table\n");
	tile_table_destroy(table);
	printf("OK!\n");
	return 0;
}
