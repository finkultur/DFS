/*
 * sched_algs.c
 *
 * Different scheduling algorithms
 *
 */

// Just for debugging
#include <stdio.h>

// Tilera
#include <tmc/cpus.h>
#include <tmc/task.h>

#include "cluster_table.h"
#include "perfcount.h"
#include "scheduling.h"

#define NUM_OF_MEMORY_DOMAINS 4

/*
 * Sets the specified timer to "pling" in timeout seconds.
 */
void set_timer(timer_t *timer, int timeout)
{
	struct itimerspec value;
	value.it_interval.tv_sec = timeout;
	value.it_interval.tv_nsec = 0;
	value.it_value = value.it_interval;
	timer_settime(*timer, 0, &value, NULL);
}

int await_processes(void)
{
	pid_t pid;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		ctable_remove(table, pid);
		// log message
	}
	return pid;
}

/*
 * Starts all processes with start_time less than or equal to the current
 * time/counter.
 *
 * For each new process it tries to get the cpu_set with the least contention
 * (might be a totally empty set).
 *
 * Derps with the pid table, allocates a specific tile
 * to the process(es) and forks child process(es). Also keeps track of the
 * next set of program to start.
 */
int run_commands(void)
{
	int fd, best_cluster;
	pid_t pid;
	cmd_t *cmd;

	while ((cmd = cmd_queue_front(queue)) != NULL && cmd->start <= time) {

		best_cluster = get_best_cluster(table);

		pid = fork();
		if (pid < 0) {
			return -1;
		} else if (pid == 0) {

			if (tmc_cpus_set_my_affinity(&clusters[best_cluster]) < 0) {
				tmc_task_die("failure in 'tmc_set_my_cpu'");
			}

			/* Change working directory. */
			chdir(cmd->dir);

			/* Redirect stdin. */
			if (cmd->input_file != NULL) {
				fd = open(cmd->input_file, O_RDONLY);
				if (fd == -1) {
					exit(EXIT_FAILURE);
				}
				dup2(fd, 0);
			}

			/* Redirect stdout. */
			if (cmd->output_file != NULL) {
				fd = open(cmd->output_file, O_CREAT);
				if (fd == -1) {
					exit(EXIT_FAILURE);
				}
				dup2(fd, 1);
			}

			/* Execute program. */
			if (execv(cmd->cmd, cmd->argv) != 0) {
				exit(EXIT_FAILURE);
			}

		} else {
			// Add pid to tile table
			ctable_insert(table, pid, best_cluster, cmd->class);
		}

		/* Remove command from queue. */
		cmd_queue_dequeue(queue);
	}
	return 0;
}

void *read_pmc(void *arg)
{
	int my_tile = (int) arg;
	int wr_miss, wr_cnt, drd_miss, drd_cnt;
	float miss_rate;

	struct timespec sleep_time;

	// Calculate row and column (to be sent to ctable)
	int row = my_tile / 8;
	int col = my_tile % 8;

	// Check if my_tile exists in cpu_set (not occupied by I/O for example)
	if (tmc_cpus_has_cpu(&online_cpus, my_tile) != 0) {
		return NULL;
	}

	// Set affinity to that cpu
	if (tmc_cpus_set_my_cpu(my_tile) < 0) {
		printf("failure for tile %i\n", my_tile);
		tmc_task_die("failure in 'tmc_set_my_cpu'");
	}

	// Setup counters
	clear_counters();
	setup_counters(LOCAL_WR_MISS, LOCAL_WR_CNT, LOCAL_DRD_MISS, LOCAL_DRD_CNT);

	sleep_time.tv_sec = POLLING_INTERVAL;
	sleep_time.tv_nsec = 0;

	// Loop until all applications are terminated, update miss rate table
	while (!all_terminated) {
		//printf("Hi this is %i on cpu %i\n", my_tile, tmc_cpus_get_my_current_cpu());
		read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
		clear_counters();
		// Push the data to the cluster table
		miss_rate = ((float) (wr_miss + drd_miss)) / (wr_cnt + drd_cnt);
		ctable_set_miss_rate(table, col, row, miss_rate);
		//printf("read PMC on tile: %i. miss rate: %f\n", my_tile, miss_rate);

		nanosleep(&sleep_time, NULL);
	}
	return NULL;
}


/*
 * Returns the index of an empty cluster if there is one.
 * If no cluster is empty, return -1.
 */
int get_empty_cluster(void)
{
	for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		if (ctable_get_count(table, i) == 0) {
			return i;
		}
	}
	return -1;
}

int get_best_cluster(void)
{
	int empty_cluster = get_empty_cluster(table);
	if (empty_cluster >= 0) {
		return empty_cluster;
	} else {
		return best_cluster;
	}
}

/*
 * Total miss rate for ALL clusters
 */
float calc_total_miss_rate(float **miss_rates)
{
	float total_miss_rate = 0;
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			total_miss_rate += miss_rates[x][y];
		}
	}
	return total_miss_rate;
}


/*
 * Total miss rate per cluster
 */
float calc_cluster_miss_rate(float **miss_rates, int cluster)
{
	int init = (cluster % 2) * 4;
	float cluster_miss_rate = 0;
	for (int row = init; row <= (init + 4); row++) {
		for (int col = init; col <= (init + 4); col++) {
			cluster_miss_rate += miss_rates[col][row];
		}
	}
	return cluster_miss_rate;
}



/*
 * Checks tile table for an empty cluster or the cluster
 * with least miss rate.
 */
int get_cluster_with_min_contention(ctable_t *table)
{
	float *miss_rates = table.cluster_miss_rate;

	int best_cluster = 0;
	float best_cluster_contention = miss_rates[0];

	for (int i = 1; i < NUM_OF_MEMORY_DOMAINS; i++) {
		if (tile_miss_rates[i] < best_cluster_contention) {
			best_cluster = i;
		}
	}

	return best_cluster;
}


/*
 * Checks if there is a need to move pids from one cluster to another.
 * Calculates the average miss rate by cluster and compares every clusters
 * value to a limit specified by X*avg_miss_rate.
 * If the limit is met, migrate a pid to a new cluster and rearrange tile_table.
 */
void run_scheduler(void)
{
	int new_cluster;
	float cluster_miss_rate;
	float total_miss_rate;
	float avg_miss_rate;
	float limit;
	float **miss_rates;

	miss_rates = ctable_get_miss_rates(table);

	total_miss_rate = calc_total_miss_rate(miss_rates);

	avg_miss_rate = total_miss_rate / NUM_OF_MEMORY_DOMAINS;

	limit = 1.5 * avg_miss_rate;

	for (i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {
		cluster_miss_rate = calc_cluster_miss_rate(miss_rates, i);
		ctable_set_cluster_miss_rate(table, i, cluster_miss_rate);
	}


	for (int i = 0; i < NUM_OF_MEMORY_DOMAINS; i++) {

		if (calc_cluster_miss_rate(miss_rates, i) > limit && ctable_get_count(
				table, i) > 1) {

			new_cluster = get_cluster_with_min_contention(table);

			pid_t pid_to_move = ctable_get_minimum_pid(table, i);

			if (tmc_cpus_set_my_affinity(clusters[new_cluster])) {
				tmc_task_die("sched_algs: Failure in tmc_cpus_set_my_affinity");
			}

			ctable_migrate(table, pid_to_move, new_cluster);
		}

	}

}
