/* scheduling.c
 *
 * Scheduling routines. */

#include <fcntl.h>
#include <float.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "pmc.h"
#include "scheduling.h"

// HELPER FUNCTIONS:
static int best_cluster(void);

static float calc_cluster_miss_rate(int cluster);

// DEFINITIONS

int init_scheduler(char *workload) {
	int i, row, column;

	pthread_attr_t thread_attr;
	pthread_t pmc_threads[TILE_COUNT];

	all_started = 0;

	all_terminated = 0;

	active_cluster_count = 0;

	// Save all online cpus to online_cpus
	tmc_cpus_get_online_cpus(&online_set);
	// Add 4 different grids with size 4x4 to different cpu_sets
	tmc_cpus_grid_add_rect(&cluster_sets[0], 0, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[1], 4, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[2], 0, 4, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[3], 4, 4, 4, 4);
	// Remove the tiles not online (e.g. reserved for I/O)
	tmc_cpus_intersect_cpus(&cluster_sets[0], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[1], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[2], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[3], &online_set);


	for (i = 0; i < TILE_CLUSTERS; i++) {
		if (tmc_cpus_count(&cluster_sets[i]) > 0) {
			active_cluster_count++;
			active_clusters[i] = 1;
		}
		else {
			active_clusters[i] = 0;
		}
		pid_counters[i] = 0;
		cluster_rates[i] = 0.0f;
	}

	for (row = 0; row < TILE_ROWS; row++) {
		for (column = 0; column < TILE_COLUMNS; column++) {
			tile_rates[row][column] = 0.0f;
		}
	}


	// Initialize command queue
	cmd_queue = cmd_queue_create(workload);
	if (cmd_queue == NULL) {
		fprintf(stderr, "Failed to create command queue\n");
		return 1;
	} else if (cmd_queue_get_size(cmd_queue) == 0) {
		fprintf(stderr, "Empty command queue\n");
		return 1;
	}

	pid_set = pid_set_create();
	if (pid_set == NULL) {
		fprintf(stderr, "Failed to create process set\n");
		return 1;
	}

	// Start polling threads for each cpu
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	for (i = 0; i < TILE_COUNT; i++) {
		// Check if my_tile exists in cpu_set (not occupied by I/O for example)
		if (tmc_cpus_has_cpu(&online_set, i) == 1) {
			pthread_create(&pmc_threads[i], &thread_attr, read_pmc, (void*)i);
		}
	}

	return 0;
}



void run_scheduler(void)
{
	int cluster, migration_cluster;
	float total_miss_rate;
	float average_miss_rate;
	float migration_limit;
	pid_t migration_pid;

	total_miss_rate = 0.0f;
	for (cluster = 0; cluster < TILE_CLUSTERS; cluster++) {
		if (pid_counters[cluster] >= 0) {
			cluster_rates[cluster] = calc_cluster_miss_rate(cluster);
			total_miss_rate += cluster_rates[cluster];
		}
	}

	average_miss_rate = total_miss_rate / active_cluster_count;

	migration_limit = 1.5 * average_miss_rate;

	for (cluster = 0; cluster < TILE_CLUSTERS; cluster++) {

		// Check if cluster is active

		if (cluster_rates[cluster] > migration_limit) {
			migration_cluster = best_cluster();
			migration_pid = pid_set_get_minimum_pid(pid_set, cluster);

			if (tmc_cpus_set_task_affinity(&cluster_sets[migration_cluster], migration_pid)) {
				tmc_task_die("Failure in tmc_cpus_set_my_affinity");
			}

			// MIGRATE PAGES!!!

			pid_set_set_cluster(pid_set, migration_pid, migration_cluster);
			pid_counters[cluster]--;
			pid_counters[migration_cluster]++;
		}
	}
}

int run_commands(int clock)
{
	int fd, cluster, timeout;
	pid_t pid;
	cmd_t *cmd;
	while ((cmd = cmd_queue_front(cmd_queue)) != NULL && cmd->start <= clock) {
		cluster = best_cluster();
		pid = fork();
		if (pid < 0) {
			return -1;
		} else if (pid == 0) {
			if (tmc_cpus_set_my_affinity(&cluster_sets[cluster]) < 0) {
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
			// Add pid to cluster table
			pid_set_insert(pid_set, pid, cluster, cmd->class);
			pid_counters[cluster]++;
		}
		/* Remove command from queue. */
		cmd_queue_dequeue(cmd_queue);
	}

	// Return time to next command
	if (cmd_queue_get_size(cmd_queue) > 0) {
		timeout = cmd_queue_front(cmd_queue)->start - clock;
		return timeout;
	} else {
		return -1;
	}
}

int await_processes(void)
{
	int cluster;
	pid_t pid;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		cluster = pid_set_get_cluster(pid_set, pid);
		pid_set_remove(pid_set, pid);
		pid_counters[cluster]--;
	}
	return pid;
}


void *read_pmc(void *arg)
{
	int cpu, row, column;
	int wr_miss, wr_cnt, drd_miss, drd_cnt;
	float miss_rate;
	struct timespec sleep_time;

	cpu = (int) arg;
	row = cpu / 8;
	column = cpu % 8;
	sleep_time.tv_sec = PMC_POLLING_INTERVAL;
	sleep_time.tv_nsec = 0;
	if (tmc_cpus_set_my_cpu(cpu) < 0) {
		fprintf(stderr, "Failed to start thread on cpu %i\n", cpu);
		tmc_task_die("failure in 'tmc_set_my_cpu'");
	}
	clear_counters();
	setup_counters(LOCAL_WR_MISS, LOCAL_WR_CNT, LOCAL_DRD_MISS, LOCAL_DRD_CNT);
	while (!all_terminated) {
		read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
		clear_counters();
		miss_rate = ((float) (wr_miss + drd_miss)) / (wr_cnt + drd_cnt);
		tile_rates[column][row] = miss_rate;
		nanosleep(&sleep_time, NULL);
	}
	return NULL;
}

int set_timer(timer_t *timer, int timeout)
{
	struct itimerspec value;
	value.it_interval.tv_sec = timeout;
	value.it_interval.tv_nsec = 0;
	value.it_value = value.it_interval;
	return timer_settime(*timer, 0, &value, NULL);
}

// HELPER FUNCTIONS
static int best_cluster(void)
{
	int cluster, best_cluster;
	float miss_rate, best_miss_rate;

	best_miss_rate = FLT_MAX;

	for (cluster = 0; cluster < TILE_CLUSTERS; cluster++) {
		if (pid_counters[cluster] == 0) {
			return cluster;
		} else if (pid_counters[cluster] > 0) {
			miss_rate = cluster_rates[cluster];
			if (miss_rate < best_miss_rate) {
				best_cluster = cluster;
			}
		}
	}
	return best_cluster;
}

static float calc_cluster_miss_rate(int cluster)
{
	int start, end, row, column;
	float miss_rate = 0.0f;

	start = (cluster % 2) * 4;
	for (row = start, end = row + 4; row < end; row++) {
		for (column = start, end = column + 4; column < end; column++) {
			miss_rate += tile_rates[column][row];
		}
	}
	return miss_rate;
}
