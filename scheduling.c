/* scheduling.c
 *
 * Implementation of scheduling routines. */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scheduling.h"

/* Flag values indicating when all commands from the workload have been started
 * and when all corresponding processes are terminated. */
int all_started = 0;
int all_terminated = 0;

/* Run time clock counter. */
int run_clock = 0;

/* Number of started processes. */
int start_count = 0;

/* Number of terminated processes (successful). */
int end_count = 0;

/* Command queue. */
cmd_queue_t *queue;

/* File for logging scheduler output. */
FILE *stdlog;

/* Scheduling timers. */
timer_t command_timer;

/* Start and end time for execution time measurement. */
struct timeval start_time, end_time;

/* Helper prototypes. */
static int set_timer(timer_t *timer, size_t timeout);

/* Initialize scheduler. */
int start_scheduler(char *workload, char *logfile)
{
	struct sigevent command_event;
	/* Set start time. */
	gettimeofday(&start_time, NULL);
	/* Open default or specified log file. */
	logfile = (logfile == NULL ? SCHEDULER_LOG : logfile);
	if ((stdlog = fopen(logfile, "a")) == NULL) {
		fprintf(stderr, "Failed to open log file %s\n", logfile);
		return -1;
	} else if (setvbuf(stdlog, NULL, _IOLBF, BUFSIZ) != 0) {
		fprintf(stderr, "Failed to set log file buffer\n");
		return -1;
	}
	fprintf(stdlog, "Scheduler starting\n");
	/* Initialize command queue. */
	queue = cmd_queue_create(workload);
	if (queue == NULL) {
		fprintf(stderr, "Failed to create command queue\n");
		return -1;
	} else if (cmd_queue_get_size(queue) == 0) {
		fprintf(stderr, "Empty command queue\n");
		return -1;
	}
	fprintf(stdlog, "%i commands in queue\n", cmd_queue_get_size(queue));
	/* Create command start timer using SIGALRM. */
	command_event.sigev_notify = SIGEV_SIGNAL;
	command_event.sigev_signo = SIGALRM;
	if (timer_create(CLOCK_REALTIME, &command_event, &command_timer) != 0) {
		fprintf(stderr, "Failed to create command start timer\n");
		return -1;
	}
	/* Run commands. */
	if (cmd_queue_get_size(queue) > 0) {
		run_commands();
	} else {
		all_started = 1;
		all_terminated = 1;
	}
	return 0;
}

/* Stop scheduler and release opened resources. */
int stop_scheduler(void)
{
	long msec;
	if (all_started && all_terminated) {
		/* Set end time and print execution duration. */
		gettimeofday(&end_time, NULL);
		msec = (end_time.tv_sec - start_time.tv_sec) * 1000;
		msec += (end_time.tv_usec - start_time.tv_usec) / 1000;
		fprintf(stdlog, "Scheduler stopped\n");
		fprintf(stdlog, "%i commands ended successfully\n", end_count);
		fprintf(stdlog, "Execution time: %lu,%lu seconds\n",
				msec / 1000, msec % 1000);
		/* Release allocated resources. */
		timer_delete(command_timer);
		cmd_queue_destroy(queue);
		fclose(stdlog);
		return 1;
	} else {
		return 0;
	}
}

/* Start the next set of commands in queue. */
void run_commands(void)
{
	int fd, timeout;
	pid_t pid;
	cmd_t *cmd;
	/* Start all command queued for start at the current time. */
	while (cmd_queue_get_size(queue) > 0 && cmd_queue_front(queue)->start
			<= run_clock) {
		cmd = cmd_queue_front(queue);
		fprintf(stdlog, "Starting command: %s Time: %i\n", cmd->cmd, run_clock);
		pid = fork();
		if (pid < 0) {
			fprintf(stdlog, "Failed to fork process\n");
			cmd_queue_dequeue(queue);
		} else if (pid > 0) {
			/* Remove command from queue. */
			cmd_queue_dequeue(queue);
			start_count++;
		} else {
			chdir(cmd->dir);
			/* Check redirection of stdin */
			if (cmd->input_file != NULL) {
				fd = open(cmd->input_file, O_RDONLY);
				if (fd == -1) {
					fprintf(stdlog, "Failed to redirect stdin\n");
					exit(EXIT_FAILURE);
				}
				dup2(fd, 0);
			}
			// Redirection of stdout
			if (cmd->output_file != NULL) {
				fd = open(cmd->output_file, O_CREAT);
				if (fd == -1) {
					fprintf(stdlog, "Failed to redirect stdout\n");
					exit(EXIT_FAILURE);
				}
				dup2(fd, 1);
			}
			/* Execute program. */
			if (execv(cmd->cmd, cmd->argv) != 0) {
				fprintf(stdlog, "Failed to execute command %s\n", cmd->cmd);
				exit(EXIT_FAILURE);
			}
		}
	}
	/* Set command timer of indicate all commands started. */
	if (cmd_queue_get_size(queue) > 0) {
		timeout = (cmd_queue_front(queue)->start - run_clock) * 1000;
		run_clock = cmd_queue_front(queue)->start;
		set_timer(&command_timer, timeout);
	} else {
		all_started = 1;
		set_timer(&command_timer, 0);
		fprintf(stdlog, "All commands started\n");
	}
}

/* Wait for terminated child processes. */
void await_processes(void)
{
	int status;
	pid_t pid;
	/* Await all terminated child processes. */
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		end_count++;
		fprintf(stdlog, "Process %i terminated with status %i\n", pid, status);
	}
	/* Set flag if there are no more processes to wait for. */
	if (pid < 0 && all_started == 1) {
		all_terminated = 1;
		fprintf(stdlog, "All processes terminated\n");
	}
}

/* Sets the timeout of the specified timer to the specified number of
 * milliseconds. Return 0 on success, otherwise -1. */
static int set_timer(timer_t *timer, size_t timeout)
{
	struct itimerspec value;
	value.it_interval.tv_sec = timeout / 1000;
	value.it_interval.tv_nsec = (timeout % 1000) * 1000000;
	value.it_value = value.it_interval;
	if (timer_settime(*timer, 0, &value, NULL) != 0) {
		fprintf(stdlog, "Failed to set timer\n");
		return -1;
	}
	return 0;
}
