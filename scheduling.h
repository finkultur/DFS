/*
 * scheduling.h
 *
 * Scheduling routines.
 */

#include "cmd_queue.h"
#include "pid_set.h"

// CONSTANTS
#define PMC_POLLING_INTERVAL 1
#define SCHEDULING_INTERVAL 2
#define TILE_COUNT 64
#define TILE_COLUMNS 8
#define TILE_ROWS 8
#define TILE_CLUSTERS 4

// GLOBALS
int all_started;

int all_terminated;

int active_cluster_count;

cmd_queue_t *cmd_queue;

pid_set_t *pid_set;

cpu_set_t online_set;

cpu_set_t cluster_sets[TILE_CLUSTERS];

int active_clusters[TILE_CLUSTERS];

int pid_counters[TILE_CLUSTERS];

float cluster_rates[TILE_CLUSTERS];

float tile_rates[8][8];

// PROTOTYPES:

int init_scheduler(char *workload);

void run_scheduler(void);

int run_commands(int clock);

int await_processes(void);

void *read_pmc(void *args);

int set_timer(timer_t *timer, int seconds);
