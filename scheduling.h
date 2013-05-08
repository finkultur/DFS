/*
 * sched_algs.h
 *
 * Different scheduling algorithms
 */

#include "cluster_table.h"
#include "cmd_queue.h"

#define POLLING_INTERVAL 1

#define SCHEDULING_INTERVAL 2

#define NUMBER_OF_CLUSTERS 4

#define NUMBER_OF_TILES 64

// GLOBALS
int all_started, all_terminated;

int timer;

int best_cluster;

cmd_queue_t *queue;

ctable_t *table;

cpu_set_t online_cpus;

cpu_set_t cpu_clusters[NUMBER_OF_CLUSTERS];

// PROTOTYPES:

void set_timer(timer_t *timer, int timeout);

int await_processes(void);

int run_commands(void);

void run_scheduler(void);

void *read_pmc(void *args);

int get_empty_cluster(void);

int get_best_cluster(void);

float calc_total_miss_rate(float **miss_rates);

float calc_cluster_miss_rate(float **miss_rates, int cluster);

int get_cluster_with_min_contention(ctable_t *table);
