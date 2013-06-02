/* scheduling.h
 *
 * Declarations of scheduling variables and routines. */

#ifndef _SCHEDULING_H
#define _SCHEDULING_H

#include <stdint.h>

#include "cmd_queue.h"
#include "pid_set.h"

/* TILEPro64 CPU specifications. */
#define NODE_COUNT 4
#define CPU_COUNT 64

/* Number of milliseconds between each invocation of the scheduling routine. */
#define SCHEDULING_INTERVAL 250 

/* Factor used to set a limit for when process migration should be performed. */
#define MIGRATION_FACTOR 2

/* Size of data buffers. */
#define BUFFER_SIZE 256

/* Default file for log output. */
#define SCHEDULER_LOG "run.log"

/* NUMA-node CPU maps. */
#define NODE_CPU_MAPS "/sys/devices/system/node/node%i/cpumap"

/* Memory controller profiling data. */
#define MEMPROF_DATA_FILE "/proc/tile/memprof"

/* Initializes the scheduler with the specified workload and optional log file.
 * If no log file is specified messages are logged to a default log file.
 * Returns 0 on success, otherwise -1. */
int start_scheduler(char *workload, char *logfile);

/* Terminates the scheduler. */
int stop_scheduler(void);

/* Performs the online scheduling actions. These actions involves calculating
 * the current rate of memory operations on each controller and migrating
 * processes to balance the contention if necessary. */
void run_scheduler(void);

/* Runs the commands queued for startup at the current clock time. If the
 * queue is empty, a global flag is set to indicate that all commands have been
 * started. */
void run_commands(void);

/* Removes any terminated child processes from the system. If all child
 * processes have been terminated and all commands started, a global flag is
 * set to indicate that the scheduler can terminate. */
void await_processes(void);

#endif /* _SCHEDULING_H */
