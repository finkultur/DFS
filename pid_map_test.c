/* pid_map_test.c */

#include "pid_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Test functions:
static int assert_map(pid_map_t *map);

// Perform test of pid_map module.
int main(int argc, char *argv[]) {
	int i, count, interval, pid, class, cpu;
	int *pid_vec;
	pid_map_t *map;

	if (argc != 3) {
		printf("Usage: %s <number of entries> "
			"<assert interval, 0 = disabled>\n", argv[0]);
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
	// Create pid_map:
	printf("Creating map...");
	fflush(stdout);
	map = pid_map_create();
	if (map == NULL) {
		printf("Failed\n");
		return 1;
	} else {
		printf("OK\n");
	}
	// Insert entries and assert tree properties:
	printf("Inserting %i random entries\n", count);
	for (i = 0; i < count; i++) {
		pid = rand();
		class = rand() % 4;
		cpu = rand() % 64;
		pid_vec[i] = pid;
		if (pid_map_insert(map, pid, class, cpu) == -1) {
			printf("Insert failure on entry: %i\n", i + 1);
			return 1;
		}
		// Assert if interval reached:
		if ((interval != 0) && ((i + 1) % interval == 0)) {
			if (assert_map(map) != 0) {
				return 1;
			}
		}
	}
	printf("%i entries inserted\n", count);
	// Switch classes:
	printf("Switching PID classes\n");
	for (i = 0; i < count; i++) {
		class = rand() % 4;
		if (pid_map_get_class(map, pid_vec[i]) == -1) {
			printf("Unable to get class for PID: %i\n", pid_vec[i]);
		} else if (pid_map_set_class(map, pid_vec[i], class) == -1) {
			printf("Unable to set class for PID: %i\n", pid_vec[i]);
		}
	}
	printf("All PID classes switched\n");
	// Switch CPUs:
	printf("Switching CPUs\n");
	for (i = 0; i < count; i++) {
		cpu = rand() % 64;
		if (pid_map_get_cpu(map, pid_vec[i]) == -1) {
			printf("Unable to get CPU for PID: %i\n", pid_vec[i]);
		} else if (pid_map_set_cpu(map, pid_vec[i], cpu) == -1) {
			printf("Unable to set CPU for PID: %i\n", pid_vec[i]);
		}
	}
	printf("All CPUs switched\n");
	// Remove entries and assert tree properties:
	printf("Removing all entries\n");
	for (i = 0; i < count; i++) {
		pid_map_remove(map, pid_vec[i]);
		// Assert if interval reached:
		if ((interval != 0) && ((i + 1) % interval == 0)) {
			assert_map(map);
		}
	}
	printf("All entries removed\n");
	printf("Destroying map...");
	fflush(stdout);
	pid_map_destroy(map);
	printf("OK\n");
	// Test ended:
	printf("Test completed!\n");
	return 0;
}

// Assert map:
static int assert_map(pid_map_t *map) {
	int assertion;

	printf("Asserting tree properties...");
	fflush(stdout);
	assertion = pid_map_assert_tree(map);
	if (assertion == 0) {
		printf("OK\n");
	} else if (assertion == -1) {
		printf("Assertion failure\n");
	} else {
		printf("Property violation: %i\n", assertion);
	}
	return assertion;
}
