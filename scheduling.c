/* scheduling.c
 *
 * TODO: add description.
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

/* Helper prototypes. */
static int get_optimal_cluster(void);
static float get_cluster_miss_rate(int cluster);

/* Initialize scheduler. */
int init_scheduler(char *workload) {
	int i, row, column;

	/* Initialize command queue. */
	cmd_queue = cmd_queue_create(workload);
	if (cmd_queue == NULL) {
		fprintf(stderr, "Failed to create command queue\n");
		return -1;
	} else if (cmd_queue_get_size(cmd_queue) == 0) {
		fprintf(stderr, "Empty command queue\n");
		return -1;
	}

	/* Initialize process set. */
	pid_set = pid_set_create();
	if (pid_set == NULL) {
		fprintf(stderr, "Failed to create process set\n");
		return -1;
	}

	/* Get online CPUs and initialize CPU sets. */
	tmc_cpus_get_online_cpus(&online_set);
	tmc_cpus_grid_add_rect(&cluster_sets[0], 0, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[1], 4, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[2], 0, 4, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[3], 4, 4, 4, 4);
	tmc_cpus_intersect_cpus(&cluster_sets[0], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[1], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[2], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[3], &online_set);

	/* Initialize scheduling variables. */
	all_started = 0;
	all_terminated = 0;
	run_clock = 0;
	active_cluster_count = 0;
	for (i = 0; i < CPU_CLUSTERS; i++) {
		/* Check active CPU clusters. */
		if (tmc_cpus_count(&cluster_sets[i]) > 0) {
			active_cluster_count++;
			active_clusters[i] = 1;
		}
		else {
			active_clusters[i] = 0;
		}
		cluster_pids[i] = 0;
		cluster_miss_rates[i] = 0.0f;
	}

	/* Initialize CPU miss rate matrix. */
	for (row = 0; row < CPU_ROWS; row++) {
		for (column = 0; column < CPU_COLUMNS; column++) {
			cpu_miss_rates[row][column] = 0.0f;
		}
	}

	/* Start PMC reading threads one each CPU. */
	for (i = 0; i < CPU_COUNT; i++) {
		if (tmc_cpus_has_cpu(&online_set, i) == 1 &&
			pthread_create(&pmc_threads[i], NULL, read_pmc, (void*)i) != 0) {
			fprintf(stderr, "Failed to create thread on CPU %i\n", i);
			return -1;
		} else {
			pmc_threads[i] = 0;
		}
	}
	return 0;
}

/* Perform scheduling tasks. */
void run_scheduler(void)
{
	int cluster, migration_cluster;
	float total_miss_rate;
	float average_miss_rate;
	float migration_limit;
	pid_t migration_pid;

	/* Calculate miss rates for each CPU cluster and a total over all CPU
	 * clusters. */
	total_miss_rate = 0.0f;
	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
		if (active_clusters[cluster] == 1) {
			cluster_miss_rates[cluster] = get_cluster_miss_rate(cluster);
			total_miss_rate += cluster_miss_rates[cluster];
		}
	}

	/* Calculate an avergage cluster miss rate and an upper limit for
	 * process migration. */
	average_miss_rate = total_miss_rate / active_cluster_count;
	migration_limit = average_miss_rate * MIGRATION_FACTOR;

	/* Check all clusters for process migration. */
	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
		if (active_clusters[cluster] == 0) {
			continue;
		} else if (cluster_miss_rates[cluster] > migration_limit
				&& cluster_pids[cluster] > 1) {
			migration_cluster = get_optimal_cluster();
			migration_pid = pid_set_get_minimum_pid(pid_set, cluster);
			/* Migrate process by setting its CPU cluster affinity. */
			if (tmc_cpus_set_task_affinity(&cluster_sets[migration_cluster], migration_pid)) {
				tmc_task_die("Failure in tmc_cpus_set_task_affinity()");
			}
			/* Migrate memory pages. */
			pid_set_set_cluster(pid_set, migration_pid, migration_cluster);
			cluster_pids[cluster]--;
			cluster_pids[migration_cluster]++;
		}
	}
}

/* Start the next set of commands in queue. */
int run_commands(void)
{
	int fd, cluster, timeout;
	pid_t pid;
	cmd_t *cmd;
	/* Start all command queued for start at the current time. */
	while ((cmd = cmd_queue_front(cmd_queue)) != NULL
			&& cmd->start <= run_clock) {
		cluster = get_optimal_cluster();
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Failed to fork process\n");
			return -1;
		} else if (pid == 0) {
			if (tmc_cpus_set_my_affinity(&cluster_sets[cluster]) < 0) {
				tmc_task_die("Failure in tmc_set_my_affinity()");
			}
			chdir(cmd->dir);
			/* Check redirection of stdin/stdout. */
			if (cmd->input_file != NULL) {
				fd = open(cmd->input_file, O_RDONLY);
				if (fd == -1) {
					exit(EXIT_FAILURE);
				}
				dup2(fd, 0);
			}
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
			cluster_pids[cluster]++;
		}
		/* Remove command from queue. */
		cmd_queue_dequeue(cmd_queue);
	}

	/* Return the number of seconds until the next command should be run or
	 * 0 if all commands have been started. */
	if (cmd_queue_get_size(cmd_queue) > 0) {
		timeout = cmd_queue_front(cmd_queue)->start - run_clock;
        run_clock = cmd_queue_front(cmd_queue)->start;
		return timeout;
	} else {
		all_started = 1;
		return 0;
	}
}

/* Wait for terminated child processes. */
void await_processes(void)
{
	int cluster;
	pid_t pid;
	/* Await all terminated child processes. */
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		cluster = pid_set_get_cluster(pid_set, pid);
		pid_set_remove(pid_set, pid);
		cluster_pids[cluster]--;
	}
	/* Set flag if there are no more processes to wait for. */
	if (pid == -1 && all_started == 1) {
		all_terminated = 1;
	}
}

/* Periodically read PMC register values and calculate the miss rate. */
void *read_pmc(void *arg)
{
	int cpu, row, column;
	int wr_miss, wr_cnt, drd_miss, drd_cnt;
	float miss_rate;
	struct timespec interval;

	cpu = (int) arg;
	row = cpu / CPU_ROWS;
	column = cpu % CPU_COLUMNS;
	interval.tv_sec = PMC_READ_INTERVAL;
	interval.tv_nsec = 0;
	/* Set thread affinity. */
	if (tmc_cpus_set_my_cpu(cpu) < 0) {
		fprintf(stderr, "Failed to start thread on cpu %i\n", cpu);
		tmc_task_die("failure in 'tmc_set_my_cpu'");
	}
	/* Setup PMC registers. */
	clear_counters();
	setup_counters(LOCAL_WR_MISS, LOCAL_WR_CNT, LOCAL_DRD_MISS, LOCAL_DRD_CNT);
	/* Periodically read registers, calculate miss rate and store value. */
	while (all_terminated == 0) {
		read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
		clear_counters();
		miss_rate = ((float) (wr_miss + drd_miss)) / (wr_cnt + drd_cnt);
		cpu_miss_rates[column][row] = miss_rate;
		nanosleep(&interval, NULL);
	}
	return NULL;
}

/* Sets the timeout of the specified timer. */
int set_timer(timer_t *timer, int timeout)
{
	struct itimerspec value;
	value.it_interval.tv_sec = timeout;
	value.it_interval.tv_nsec = 0;
	value.it_value = value.it_interval;
	if (timer_settime(*timer, 0, &value, NULL) != 0) {
		fprintf(stderr, "Failed to set timer\n");
		return -1;
	}
	return 0;
}

/* Finds and returns the number of the cluster with least contention. */
static int get_optimal_cluster(void)
{
	int cluster, optimal_cluster;
	float miss_rate, best_miss_rate;

	/* Find an active cluster with no running processes or select the cluster
	 * with least contention (lowest miss rate). */
	best_miss_rate = FLT_MAX;
	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
		if (active_clusters[cluster] == 0) {
			continue;
		}
		if (cluster_pids[cluster] == 0) {
			return cluster;
		} else {
			miss_rate = cluster_miss_rates[cluster];
			if (miss_rate < best_miss_rate) {
				optimal_cluster = cluster;
			}
		}
	}
	return optimal_cluster;
}

/* Returns the accumulated miss rate for all CPUs in the specified  CPU
 * cluster. */
static float get_cluster_miss_rate(int cluster)
{
	int start, end, row, column;
	float miss_rate = 0.0f;

	/* Accumulate miss rates for all CPUs in cluster. */
	start = (cluster % 2) * 4;
	for (row = start, end = row + 4; row < end; row++) {
		for (column = start, end = column + 4; column < end; column++) {
			miss_rate += cpu_miss_rates[column][row];
		}
	}
	return miss_rate;
}
