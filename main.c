/* main.c
 *
 * This is the DFS_NEXTGEN scheduler. */

#include <signal.h>
#include <stdio.h>
#include <time.h>

#include "scheduling.h"

int main(int argc, char *argv[])
{
	int signal, clock, timeout;

	sigset_t signal_mask;

	struct sigevent run_event, scheduling_event;
	timer_t run_timer, scheduling_timer;

	/* Check command line arguments. */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <inputfile>\n", argv[0]);
		return 1;
	}

	/* Set counters and flags. */
	clock = 0;
	timeout = 0;

	// Set signal masks
	if (sigemptyset(&signal_mask) != 0
			|| sigaddset(&signal_mask, SIGCHLD) != 0
			|| sigaddset(&signal_mask, SIGALRM) != 0
			|| sigaddset(&signal_mask, SIGPOLL) != 0
			|| sigprocmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
		fprintf(stderr, "Failed to set signal mask\n");
		return 1;
	}

	/* Create timers. */
	run_event.sigev_notify = SIGEV_SIGNAL;
	run_event.sigev_signo = SIGALRM;
	scheduling_event.sigev_notify = SIGEV_SIGNAL;
	scheduling_event.sigev_signo = SIGPOLL;
	if (timer_create(CLOCK_REALTIME, &run_event, &run_timer) != 0) {
		fprintf(stderr, "Failed to create command start timer\n");
		return 1;
	} else if (timer_create(CLOCK_REALTIME, &scheduling_event, &scheduling_timer) != 0) {
		fprintf(stderr, "Failed to create scheduling timer\n");
		return 1;
	}


	init_scheduler(argv[1]);

	/* Set timers. */
	set_timer(&scheduling_timer, SCHEDULING_INTERVAL);

	if ((timeout = run_commands(clock)) > 0) {
		set_timer(&run_timer, timeout);
	} else {
		all_started = 1;
		timer_delete(&run_timer);
	}

	/* Listen for signals. */
	while (!all_terminated) {
		sigwait(&signal_mask, &signal);
		switch (signal) {
		case SIGPOLL:
			run_scheduler();
			break;
		case SIGALRM:
			timeout = run_commands(clock);
			if (timeout > 0) {
				set_timer(&run_timer, timeout);
			} else {
				all_started = 1;
				timer_delete(&run_timer);
			}
			break;
		case SIGCHLD:
			if (await_processes() < 0 && all_started) {
				all_terminated = 1;
				timer_delete(&scheduling_timer);
			}
			break;
		default:
			fprintf(stderr, "Caught an invalid signal\n");
			break;
		}
	}



	return 0;
}
