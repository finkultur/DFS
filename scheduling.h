/* scheduling.h
 *
 * TODO: add description.
 * Declarations of scheduling variables and routines. */

#include "cmd_queue.h"
#include "pid_set.h"

/* Number of seconds between each invocation of the scheduler. */
#define SCHEDULING_INTERVAL 1

/* Factor used to set a limit for when process migration should be performed. */
#define MIGRATION_FACTOR 2

/* TILEPRO64 CPU specifications. */
#define CPU_CLUSTER_COUNT 4
#define CPU_COUNT 64

/* Memory controller profiling constants. */
#define MEMPROF_BUFFER_SIZE 128
#define MEMPROF_DATA_FILE "/proc/tile/memprof"

/* Flag values indicating when all commands from the workload have been started
 * and when all corresponding processes are terminated. */
int all_started;
int all_terminated;

/* Run time clock counter. */
int run_clock;

/* Number of processes running on each CPU cluster. */
int cluster_process_count[CPU_CLUSTER_COUNT];

/* Number of processes running on each CPU. */
int cpu_process_count[CPU_COUNT];

/* Memory operation counter values for each memory controller. */
unsigned long int mc_ops[CPU_CLUSTER_COUNT];

/* Memory operation rates for each memory controller. */
unsigned long int mc_rates[CPU_CLUSTER_COUNT];

/* Memprof data input file. */
FILE *memprof_file;

/* Command queue. */
cmd_queue_t *cmd_queue;

/* Set of running processes (IDs). */
pid_set_t *pid_set;

/* All online CPUs. */
cpu_set_t online_set;

/* CPU clusters. */
cpu_set_t cluster_sets[CPU_CLUSTER_COUNT];

/* Initializes the scheduler with the specified workload file. Returns 0 on
 * success, otherwise -1. */
int init_scheduler(char *workload);

/* Performs the scheduling actions. These actions involves calculating the
 * current rate of memory operations on each controller and migrating processes
 * to balance the contention if necessary. */
void run_scheduler(void);

/* Starts the commands queued for startup at the current clock time. On success
 * the number of seconds until the next set of commands should be started is
 * returned. If the queue is empty, a global flag is set to indicate that all
 * commands have been started and 0 is returned. On failure -1 is returned. */
int run_commands(void);

/* Removes any terminated child processes from the system. If all child
 * processes have been terminated and all commands started, a global flag is
 * set to indicate that the scheduler can terminate. */
void await_processes(void);

/* Sets the timeout of the specified timer to the specified number of seconds.
 * Return 0 on success, otherwise -1. */
int set_timer(timer_t *timer, int seconds);
