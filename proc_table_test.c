/* pid_table_test.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "proc_table.h"

static const int index_size = 100;
static const int bucket_count = 10;
static const int num_entry = 100000;
static const int num_cpu = 64;

// Performs a test of the pid_table module:
int main(void) {
	int n, pid, cpu, new_cpu;

    pid_t pids[num_entry];

    printf("Creating proc table\n");
    proc_table table = create_proc_table(num_cpu);
	if (table == NULL)
	{
		printf("failed!\n");
		return 1;
	}
	printf("OK!\n");
	// Seed rand():
	srand(time(NULL));
	// Add entries:
	printf("adding %i entries\n", num_entry);
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
	printf("switching all cpus\n");
	for (n = 0; n < num_entry; n++) {
		new_cpu = rand() % 64;
		if (move_pid_to_tile(table, pids[n], new_cpu) != 0) {
			printf("failed!\n");
			return 1;
		}
	}
	printf("OK!\n");
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