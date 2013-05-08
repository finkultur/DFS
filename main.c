/* main.c
 *
 * This is the DFS scheduler. */

#include <stdio.h>
#include <tmc/task.h>

#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <tmc/cpus.h>

#include "scheduling.h"

int main(int argc, char *argv[])
{
	int i, timeout, signal;
	sigset_t signal_mask;
	struct sigevent run_commands_event, scheduling_event;
	timer_t run_command_timer, scheduling_timer;
	time_t start_time, end_time;
	pthread_attr_t thread_attr;
	pthread_t polling_threads[NUMBER_OF_TILES];

	// Get start time
	start_time = time(NULL);

	// Check command line arguments
	if (argc != 2) {
		printf("Usage: %s <inputfile>\n", argv[0]);
		return 1;
	}

	// Initialize cpu set
//	if (tmc_cpus_get_my_affinity(&online_cpus) != 0) {
//		tmc_task_die("failed to get cpu set\n");
//	}

	// Save all online cpus to online_cpus
	tmc_cpus_get_online_cpus(&online_cpus);

	// Add 4 different grids with size 4x4 to different cpu_sets
	tmc_cpus_grid_add_rect(&cpu_clusters[0], 0, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cpu_clusters[1], 4, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cpu_clusters[2], 0, 4, 4, 4);
	tmc_cpus_grid_add_rect(&cpu_clusters[3], 4, 4, 4, 4);

	// Remove the tiles not online (e.g. reserved for I/O)
	tmc_cpus_intersect_cpus(&cpu_clusters[0], &online_cpus);
	tmc_cpus_intersect_cpus(&cpu_clusters[1], &online_cpus);
	tmc_cpus_intersect_cpus(&cpu_clusters[2], &online_cpus);
	tmc_cpus_intersect_cpus(&cpu_clusters[3], &online_cpus);

//	char str0[1024], str1[1024], str2[1024], str3[1024];
//	tmc_cpus_to_string(&cpu_clusters[0], str0, 1024);
//	printf("C0 to string: %s, %i cpus\n", str0, tmc_cpus_count(&cpu_clusters[0]));
//
//	tmc_cpus_to_string(&cpu_clusters[1], str1, 1024);
//	printf("C1 to string: %s, %i cpus\n", str1, tmc_cpus_count(&cpu_clusters[1]));
//
//	tmc_cpus_to_string(&cpu_clusters[2], str2, 1024);
//	printf("C2 to string: %s, %i cpus\n", str2, tmc_cpus_count(&cpu_clusters[2]));
//
//	tmc_cpus_to_string(&cpu_clusters[3], str3, 1024);
//	printf("C3 to string: %s, %i cpus\n", str3, tmc_cpus_count(&cpu_clusters[3]));

	// Initialize cluster table
	table = ctable_create();
	if (table == NULL) {

		return -1;
	}

	// Initialize command queue
	queue = cmd_queue_create(argv[1]);
	if (queue == NULL) {

		return 1;
	}

	// Set signal masks
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGCHLD);
	sigaddset(&signal_mask, SIGALRM);
	sigaddset(&signal_mask, SIGPOLL);
	sigprocmask(SIG_BLOCK, &signal_mask, NULL);

	/* Create timers. */
	run_commands_event.sigev_notify = SIGEV_SIGNAL;
	run_commands_event.sigev_signo = SIGALRM;
	timer_create(CLOCK_REALTIME, &run_commands_event, &run_command_timer);

	scheduling_event.sigev_notify = SIGEV_SIGNAL;
	scheduling_event.sigev_signo = SIGPOLL;
	timer_create(CLOCK_REALTIME, &scheduling_event, &scheduling_timer);

	/* Set counters and flags. */
	best_cluster = 0;
	timer = 0;
	timeout = 0;

	all_started = 0;
	all_terminated = 0;

	// Start a polling threads for each cpu
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	for (i = 0; i < NUMBER_OF_TILES; i++) {
		pthread_create(&polling_threads[i], &thread_attr, read_pmc, (void*) i);
	}

	/* Start the first process(es) from the command queue. */
	run_commands();

	/* Set timers. */
	if (cmd_queue_get_size(queue) > 0) {
		timeout = cmd_queue_front(queue)->start - timer;
		set_timer(&run_command_timer, timeout);
		set_timer(&scheduling_timer, SCHEDULING_INTERVAL);
	} else {
		all_started = 1;
	}

	/* Listen for signals. */
	while (!all_terminated) {
		sigwait(&signal_mask, &signal);

		switch (signal) {
		case SIGCHLD:
			if (await_processes() < 0 && all_started) {
				all_terminated = 1;
			}
			break;
		case SIGPOLL:

			run_scheduler();

			break;
		case SIGALRM:
			timer = timer + timeout;
			if (run_commands() < 0) {
				//log message
			}
			if (cmd_queue_get_size(queue) > 0) {
				timeout = cmd_queue_front(queue)->start - timer;
				set_timer(&run_command_timer, timeout);
			} else {
				all_started = 1;
				timer_delete(run_command_timer);
			}
			break;
		default:

			break;
		}
	}

	end_time = time(NULL);

	printf("Execution time: %f\n", difftime(end_time, start_time));

	return 0;
}
