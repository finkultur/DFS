/* main.c
 *
 *  Start routine for the scheduler. Initializes the scheduler and then uses
 *  two different timers to drive the execution of commands and scheduling
 *  tasks. Log messages are written either to a default log file or the log file
 *  specified on the command line. */

#include <signal.h>
#include <stdio.h>

#include "scheduling.h"

/* Program start. */
int main(int argc, char *argv[])
{
	int signal;
	sigset_t signal_mask;
	/* Set signal mask, blocking SIGCHLD, SIGALRM and SIGPOLL. */
	if (sigemptyset(&signal_mask) != 0
			|| sigaddset(&signal_mask, SIGCHLD) != 0
			|| sigaddset(&signal_mask, SIGALRM) != 0
			|| sigprocmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
		fprintf(stderr, "Failed to set signal mask\n");
		return 1;
	}
	/* Check for valid command line arguments and initialize scheduler. */
	if (argc == 2) {
		if (start_scheduler(argv[1], NULL) != 0) {
			fprintf(stderr, "Failed to initialize schedulder\n");
			return 1;
		}
	} else if (argc == 3) {
		if (start_scheduler(argv[1], argv[2]) != 0) {
			fprintf(stderr, "Failed to initialize schedulder\n");
			return 1;
		}
	} else {
		fprintf(stderr, "Usage: %s <workload file> [logfile]\n", argv[0]);
		return 1;
	}
	/* Wait for signals and take appropriate actions until all commands
	 * have terminated. */
	while (!stop_scheduler()) {
		sigwait(&signal_mask, &signal);
		switch (signal) {
		case SIGALRM:
			run_commands();
			break;
		case SIGCHLD:
			await_processes();
			break;
		default:
			break;
		}
	}
	return 0;
}
