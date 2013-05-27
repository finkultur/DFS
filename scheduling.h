/* scheduling.h
 *
 * Declarations of scheduling variables and routines. */

#ifndef _SCHEDULING_H
#define _SCHEDULING_H

#include <stdint.h>

#include "cmd_queue.h"

/* Default file for log output. */
#define SCHEDULER_LOG "run.log"

/* Initializes the scheduler with the specified workload and optional log file.
 * If no log file is specified messages are logged to a default log file.
 * Returns 0 on success, otherwise -1. */
int start_scheduler(char *workload, char *logfile);

/* Terminates the scheduler. */
int stop_scheduler(void);

/* Runs the commands queued for startup at the current clock time. If the
 * queue is empty, a global flag is set to indicate that all commands have been
 * started. */
void run_commands(void);

/* Removes any terminated child processes from the system. If all child
 * processes have been terminated and all commands started, a global flag is
 * set to indicate that the scheduler can terminate. */
void await_processes(void);

#endif /* _SCHEDULING_H */
