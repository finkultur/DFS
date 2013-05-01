/* pid_map_test.c */

#include "pid_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Eclipse-cdt code formatter hack.
#undef NULL
#define NULL 0

// Test functions:
static int assert_map(pid_map_t *map);
static int auto_test(void);
static int manual_test(void);

// Perform test of pid_map.h module:
int main(int argc, char *argv[]) {
	int mode;
	char start_menu[] = "*** PID MAP TEST ***\n"
			"1: Automatic\n"
			"2: Manual\n"
			"0: Quit\n"
			"Select mode: ";

	// Select test and run:
	do {
		printf(start_menu);
		scanf("%i", &mode);
		switch (mode) {
		case 0:
			break;
		case 1:
			return auto_test();
		case 2:
			return manual_test();
		default:
			mode = -1;
			break;
		}
	} while (mode != 0);
	return 0;
}

// Assert map:
static int assert_map(pid_map_t *map) {
	int assertion;

	printf("Asserting tree properties...");
	fflush(stdout);
	assertion = pmap_assert_map_properties(map);
	if (assertion == 0) {
		printf("OK\n");
	} else if (assertion == -1) {
		printf("Assertion failure\n");
	} else {
		printf("Property violation: %i\n", assertion);
	}
	return assertion;
}

// Automatic test:
static int auto_test(void) {
	int i, count, interval, pid, cpu;
	int *pid_vec;

	// Read number of entries to create:
	printf("Number of entries: ");
	scanf("%i", &count);
	if (count <= 0) {
		printf("Invalid number of entries\n");
		return 1;
	}
	// Read assertion interval:
	printf("Assertion interval (0 = disabled): ");
	scanf("%i", &interval);
	if (interval < 0) {
		printf("Invalid interval\n");
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
	pid_map_t *map = pmap_create();
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
		cpu = rand() % 64;
		pid_vec[i] = pid;
		if (pmap_insert(map, pid, cpu) == -1) {
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
	// Switch CPUs:
	printf("Switching CPUs\n");
	for (i = 0; i < count; i++) {
		cpu = rand() % 64;
		if (pmap_get_cpu(map, pid_vec[i]) == -1) {
			printf("Unable to get CPU for PID: %i\n", pid_vec[i]);
		} else if (pmap_set_cpu(map, pid_vec[i], cpu) == -1) {
			printf("Unable to set CPU for PID: %i\n", pid_vec[i]);
		}
	}
	// Remove entries and assert tree properties:
	printf("Removing entries\n");
	for (i = 0; i < count; i++) {
		pmap_remove(map, pid_vec[i]);
		// Assert if interval reached:
		if ((interval != 0) && ((i + 1) % interval == 0)) {
			assert_map(map);
		}
	}
	printf("Destroying map...");
	fflush(stdout);
	pmap_destroy(map);
	printf("OK\n");
	// Test ended:
	printf("Test completed!\n");
	return 0;
}

// Perform manual test:
static int manual_test(void) {
	int option, pid, cpu, op_stat;
	pid_map_t *map;
	char option_menu[] = "1: Insert\n"
			"2: Remove\n"
			"3: Assert\n"
			"0: Quit\n"
			"Select operation: ";

	// Create pid_map:
	map = pmap_create();
	if (map == NULL) {
		printf("Failed to create pid_map\n");
		return 1;
	}
	// Perform operations:
	do {
		system("clear");
		printf(option_menu);
		scanf("%i", &option);
		switch (option) {
		case 0:	// Quit
			break;
		case 1:	// Insert
			printf("<PID> <CPU>: ");
			scanf("%i %i", &pid, &cpu);
			op_stat = pmap_insert(map, pid, cpu);
			if (op_stat == 0) {
				printf("OK\n");
			} else if (op_stat == 1) {
				printf("Duplicate entry\n");
			} else {
				printf("Failed\n");
			}
			break;
		case 2:	// Remove
			printf("<PID>: ");
			scanf("%i", &pid);
			op_stat = pmap_remove(map, pid);
			if (op_stat == 0) {
				printf("OK\n");
			} else {
				printf("Failed\n");
			}
			break;
		case 3:	// Assert
			assert_map(map);
			break;
		default:
			option = -1;
			break;
		}
		printf("Press enter to continue...");
		getchar();
		getchar();
	} while (option != 0);
	pmap_destroy(map);
	return 0;
}
