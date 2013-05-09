/* scheduling.h
 *
 * TODO: add description.
 * Declarations of scheduling variables and routines. */

#include "cmd_queue.h"
#include "pid_set.h"

/* Interval lengths for reading of PMC registers and scheduling. */
#define PMC_READ_INTERVAL 1
#define SCHEDULING_INTERVAL 2

/* Factor used to set a limit for when process migration should be performed. */
#define MIGRATION_FACTOR 1.1f

/* TILEPRO64 CPU specifications. */
#define CPU_COUNT 64
#define CPU_CLUSTERS 4
#define CPU_COLUMNS 8
#define CPU_ROWS 8

/* Flag values indicating when all commands from the workload have been started
 * and when all corresponding processes are terminated. */
int all_started;
int all_terminated;

/* Run time clock counter. */
int run_clock;

/* Number of active CPU clusters. */
int active_cluster_count;

/* Indicates which CPU clusters are active (0 = inactive, 1 = active). */
int active_clusters[CPU_CLUSTERS];

/* Number of processes running on each CPU cluster. */
int cluster_pids[CPU_CLUSTERS];

/* Last calculated miss rate for each CPU cluster. */
float cluster_miss_rates[CPU_CLUSTERS];

/* Last calculated miss rate for each CPU. */
float cpu_miss_rates[CPU_COLUMNS][CPU_ROWS];

/* Command queue. */
cmd_queue_t *cmd_queue;

/* Set of running processes (IDs). */
pid_set_t *pid_set;

/* All online CPUs. */
cpu_set_t online_set;

/* CPU clusters. */
cpu_set_t cluster_sets[CPU_CLUSTERS];

/* PMC register polling threads. */
pthread_t pmc_threads[CPU_COUNT];

/* Initializes the scheduler with the specified workload file. Returns 0 on
 * success, otherwise -1. */
int init_scheduler(char *workload);

/* Performs the scheduling actions. These actions involves calculating the
 * current miss rate on each CPU cluster and migrating processes to balance the
 * contention if necessary. */
void run_scheduler(void);

/* Starts the commands queued for startup at the current clock time. On success
 * the number of seconds until the next set of commands should be started is
 * returned. If the queue is empty, 0 is returned and on failure -1. */
int run_commands(void);

/* Removes any terminated child processes from the system. */
void await_processes(void);

/* Reads the PMC register values and calculates the miss rate on the CPU where
 * the function is executed. Meant to be executed in a separate thread on each
 * of the TILEPRO64 CPUs. */
void *read_pmc(void *args);

/* Sets the timeout of the specified timer to the specified number of seconds.
 * Return 0 on success, otherwise -1. */
int set_timer(timer_t *timer, int seconds);
