/* main.c
 *
 * TODO: add description.
 * Startup routine for the scheduler. */

#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "scheduling.h"

/* Program start. */
int main(int argc, char *argv[])
{
	int timeout, signal;
	unsigned long msec;
	sigset_t signal_mask;
	struct sigevent command_event, scheduling_event;
	timer_t command_timer, scheduling_timer;
	struct timeval start_time, end_time;

	/* Set start time. */
	gettimeofday(&start_time, NULL);

	/* Check for valid command line arguments. */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <workload file>\n", argv[0]);
		return 1;
	}

	/* Initialize scheduler.  */
	if (init_scheduler(argv[1]) != 0) {
		fprintf(stderr, "Failed to initialize scheduler\n");
		return 1;
	}

	/* Set signal mask, blocking SIGCHLD, SIGALRM and SIGPOLL. */
	if (sigemptyset(&signal_mask) != 0
			|| sigaddset(&signal_mask, SIGCHLD) != 0
			|| sigaddset(&signal_mask, SIGALRM) != 0
			|| sigaddset(&signal_mask, SIGPOLL) != 0
			|| sigprocmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
		fprintf(stderr, "Failed to set signal mask\n");
		return 1;
	}

	/* Create command start timer using SIGALRM. */
	command_event.sigev_notify = SIGEV_SIGNAL;
	command_event.sigev_signo = SIGALRM;
	if (timer_create(CLOCK_REALTIME, &command_event, &command_timer) != 0) {
		fprintf(stderr, "Failed to create command start timer\n");
		return 1;
	}

	/* Create scheduling interval timer using SIGPOLL. */
	scheduling_event.sigev_notify = SIGEV_SIGNAL;
	scheduling_event.sigev_signo = SIGPOLL;
	if (timer_create(CLOCK_REALTIME, &scheduling_event, &scheduling_timer) != 0) {
		fprintf(stderr, "Failed to create scheduling timer\n");
		return 1;
	}

	/* Set scheduling timer. */
	set_timer(&scheduling_timer, SCHEDULING_INTERVAL);

	/* Set command startup timer. */
	if ((timeout = run_commands()) > 0) {
		set_timer(&command_timer, timeout * 1000);
	} else {
		timer_delete(command_timer);
	}

	/* Listen for signals and take appropriate actions until all commands
	 * are terminated. */
	while (all_terminated == 0) {
		sigwait(&signal_mask, &signal);
		switch (signal) {
		case SIGPOLL:
			run_scheduler();
			break;
		case SIGALRM:
			timeout = run_commands();
			if (timeout > 0) {
				set_timer(&command_timer, timeout * 1000);
            }
			break;
		case SIGCHLD:
			await_processes();
            if (all_terminated) {
                timer_delete(scheduling_timer);
            }
			break;
		default:
			fprintf(stderr, "Caught an unknown signal\n");
			break;
		}
	}

	/* Set end time. */
	gettimeofday(&end_time, NULL);
	msec = (end_time.tv_sec - start_time.tv_sec) * 1000;
	msec += (end_time.tv_usec - start_time.tv_usec) / 1000;

	printf("Execution time: %lu,%lu\n", msec / 1000, msec % 1000);

	return 0;
}
