/* pid_table_test.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "proc_table.h"

static const int num_entry = 20;
static const int num_cpu = 10;

// Performs a test of the pid_table module:
int main(void) {
	int n, pid, cpu, new_cpu;

	pid_t pids[num_entry];

	printf("Creating proc table\n");
	proc_table table = create_proc_table(num_cpu);
	if (table == NULL ) {
		printf("failed!\n");
		return 1;
	}
	printf("OK!\n");
	// Seed rand():
	srand(time(NULL ));

	// Add entries:
	printf("Adding %i entries\n", num_entry);
	for (n = 0; n < num_entry; n++) {
		pid = rand();
		pids[n] = pid;
		cpu = rand() % num_cpu;
		if (add_pid(table, pid, cpu) != 0) {
			printf("failed!\n");
			return 1;
		}
	}
	printf("OK!\n");

	// Finding all pids
	printf("Check if all pids are accessible on their tiles\n");
	for (n = 0; n < num_cpu; n++) {
		int pid_count = get_pid_count(table, n);
		if (pid_count == 0) {
			printf("Empty tile\n");
		}
		pid_t array[pid_count];

		if (get_pid_vector(table, n, array, pid_count) != pid_count) {
			printf("Mismatching returned pids and number of pids on tile\n");
		}
	}
	printf("OK!\n");

	// See if pids have been moved.
	printf("Printing pids\n");
	for (n = 0; n < num_entry; n++) {
		int tile_num = get_tile_num(table, pids[n]);
		printf("Pid %u is on tile %u\n", pids[n], tile_num);
	}
	printf("All pids printed\n\n");

	// Moving all elements
	printf("Switching all cpus\n");
	for (n = 0; n < num_entry; n++) {
		new_cpu = rand() % num_cpu;
		if (move_pid_to_tile(table, pids[n], new_cpu) != 0) {
			printf("failed!\n");
			return 1;
		}
	}
	printf("OK!\n");

	// See if pids have been moved.
	printf("Printing pids\n");
	for (n = 0; n < num_entry; n++) {
		int tile_num = get_tile_num(table, pids[n]);
		printf("Pid %u is on tile %u\n", pids[n], tile_num);
	}
	printf("All pids printed\n\n");

	// Remove elements:
	printf("removing each entry\n");
	for (n = 0; n < num_entry; n++) {
		if (remove_pid(table, pids[n]) != 0) {
			printf("failed!\n");
			return 1;
		}
	}
	printf("OK!\n");
	printf("destroying table\n");
	destroy_proc_table(table);
	printf("OK!\n");
	return 0;
}
