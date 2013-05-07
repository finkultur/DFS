/* pid_set_test.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pid_set.h"

#define CPU_COUNT 64
#define CLASS_COUNT 4

// Test functions:
static int assert_set(pid_set_t *set);

// Perform test of pid_set module.
int main(int argc, char *argv[])
{
	int i, count, interval, pid, cpu, class, stat;
	int *pid_vec;
	pid_set_t *set;
	if (argc != 3) {
		printf("Usage: %s <number of entries> "
			"<assert interval, 0 = disabled>\n", argv[0]);
		return 1;
	}
	// Read number of entries to create:
	count = atoi(argv[1]);
	if (count <= 0) {
		printf("Invalid number of entries\n");
		return 1;
	}
	// Read assertion interval:
	interval = atoi(argv[2]);
	if (interval < 0) {
		printf("Invalid assertion interval\n");
		return 1;
	}
	// Allocate entry storage:
	pid_vec = malloc(count * sizeof(int));
	if (pid_vec == NULL) {
		printf("Allocation failure\n");
		return 1;
	}
	srand(time(NULL));
	// Create pid_set:
	printf("Creating set...");
	fflush(stdout);
	set = pid_set_create();
	if (set == NULL) {
		printf("Failed\n");
		return 1;
	} else {
		printf("OK\n");
	}
	// Insert entries and assert tree properties:
	printf("Inserting %i random entries\n", count);
	printf("Set size: %zu\n", pid_set_get_size(set));
	for (i = 0; i < count; i++) {
		pid = rand();
		cpu = rand() % CPU_COUNT;
		class = rand() % CLASS_COUNT;
		pid_vec[i] = pid;
		stat = pid_set_insert(set, pid, cpu, class);
		if (stat == -1) {
			printf("Insert failure on entry: %i\n", i + 1);
			return 1;
		} else if (stat == 1) {
			printf("Duplicate entry: %i\n", pid);
		}
		// Assert if interval reached:
		if ((interval != 0) && ((i + 1) % interval == 0)) {
			if (assert_set(set) != 0) {
				return 1;
			}
		}
	}
	printf("%i entries inserted\n", count);
	printf("Set size: %zu\n", pid_set_get_size(set));
	// Switch classes:
	printf("Switching classes\n");
	for (i = 0; i < count; i++) {
		class = rand() % CLASS_COUNT;
		if (pid_set_get_class(set, pid_vec[i]) == -1) {
			printf("Unable to get class for PID: %i\n", pid_vec[i]);
		} else if (pid_set_set_class(set, pid_vec[i], class) == -1) {
			printf("Unable to set class for PID: %i\n", pid_vec[i]);
		}
	}
	printf("All classes switched\n");
	// Switch CPUs:
	printf("Switching CPUs\n");
	for (i = 0; i < count; i++) {
		cpu = rand() % CPU_COUNT;
		if (pid_set_get_cluster(set, pid_vec[i]) == -1) {
			printf("Unable to get CPU for PID: %i\n", pid_vec[i]);
		} else if (pid_set_set_cpus(set, pid_vec[i], cpu) == -1) {
			printf("Unable to set CPU for PID: %i\n", pid_vec[i]);
		}
	}
	printf("All CPUs switched\n");
	// Remove entries and assert tree properties:
	printf("Removing all entries\n");
	printf("Set size: %zu\n", pid_set_get_size(set));
	for (i = 0; i < count; i++) {
		pid_set_remove(set, pid_vec[i]);
		// Assert if interval reached:
		if ((interval != 0) && ((i + 1) % interval == 0)) {
			assert_set(set);
		}
	}
	printf("All entries removed\n");
	printf("Set size: %zu\n", pid_set_get_size(set));
	printf("Destroying set...");
	fflush(stdout);
	pid_set_destroy(set);
	printf("OK\n");
	// Test ended:
	printf("Test completed!\n");
	return 0;
}

// Assert set:
static int assert_set(pid_set_t *set)
{
	int assertion;
	printf("Asserting tree properties...");
	fflush(stdout);
	assertion = pid_set_assert_set(set);
	if (assertion == 0) {
		printf("OK\n");
	} else if (assertion == -1) {
		printf("Assertion failure\n");
	} else {
		printf("Property violation: %i\n", assertion);
	}
	return assertion;
}
