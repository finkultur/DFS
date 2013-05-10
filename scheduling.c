/* scheduling.c
 *
 * TODO: add description.
 * Scheduling routines. */

#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <arch/cycle.h>

#include <sys/types.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include <numa.h>
#include <numaif.h> // Not sure if this is used.
#include "pmc.h"
#include "scheduling.h"

/* Helper prototypes. */
static int get_optimal_cluster(void);
static int get_optimal_cpu(int cluster);
static int get_cluster_miss_rate(int cluster);
static int migrate_memory(pid_t pid, int old_cluster, int new_cluster);

/* Initialize scheduler. */
int init_scheduler(char *workload)
{
	int i;

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

	/* Initialize NUMA-stuff */
	if (numa_available() < 0 || numa_max_node() == 0) {
		fprintf(stderr, "Failed to initialize NUMA\n");
		return -1;
	}

	/* Initialize scheduling variables. */
	all_started = 0;
	all_terminated = 0;
	run_clock = 0;
	//	active_cluster_count = 0;
	for (i = 0; i < CPU_CLUSTERS; i++) {
		/* Check active CPU clusters. */
		//		if (tmc_cpus_count(&cluster_sets[i]) > 0) {
		//			active_cluster_count++;
		//			active_clusters[i] = 1;
		//		}
		//		else {
		//			active_clusters[i] = 0;
		//		}
		cluster_pids[i] = 0;
		cluster_miss_rates[i] = 0;
	}

	/* Initialize CPU miss rate array */
	for (i = 0; i < CPU_COUNT; i++) {
		cpu_miss_rates[i] = 0;
		if (tmc_cpus_has_cpu(&online_set, i)) {
			cpu_pids[i] = 0;
		} else {
			cpu_pids[i] = -1;
		}
	}

	/* Start PMC reading threads one each CPU. */
	for (i = 0; i < CPU_COUNT; i++) {
		if (tmc_cpus_has_cpu(&online_set, i) == 1 && pthread_create(
				&pmc_threads[i], NULL, read_pmc, (void*) i) != 0) {
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
	int cluster, migration_cluster, migration_cpu, old_cpu;
	int total_miss_rate;
	int average_miss_rate;
	int migration_limit;
	pid_t migration_pid;

	/* Calculate miss rates for each CPU cluster and a total over all CPU
	 * clusters. */
	total_miss_rate = 0;
	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
		cluster_miss_rates[cluster] = get_cluster_miss_rate(cluster);
		total_miss_rate += cluster_miss_rates[cluster];
	}

	/* Calculate an average cluster miss rate and an upper limit for
	 * process migration. */
	average_miss_rate = total_miss_rate / CPU_CLUSTERS;
//	printf("avg miss rate is %i\n", average_miss_rate);
	migration_limit = average_miss_rate * MIGRATION_FACTOR;
//	printf("migration limit is %i\n", migration_limit);

	/* Check all clusters for process migration. */
	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
		if (cluster_miss_rates[cluster] > migration_limit
				&& cluster_pids[cluster] > 1) {
			migration_cluster = get_optimal_cluster();
            migration_cpu = get_optimal_cpu(migration_cluster);
			//printf("optimal cluster seems to be %i\n", migration_cluster);
			if (migration_cluster == cluster) {
				printf("new and old cluster was the same :(\n");
				return;
			}
			migration_pid = pid_set_get_minimum_pid(pid_set, cluster);
            old_cpu = pid_set_get_cpu(pid_set, migration_pid);
			//printf("pid to migrate is %i", migration_pid);
			// Migrate process by setting its CPU cluster affinity.
			//if (tmc_cpus_set_task_affinity(&cluster_sets[migration_cluster],
			//		migration_pid) < 0) {
			//	tmc_task_die("Failure in tmc_cpus_set_task_affinity()");
			//}
			if (tmc_cpus_set_my_cpu(migration_cpu) < 0) {
                tmc_task_die("Failure in tmc_cpus_set_my_cpu()");
            } 
			// Migrate memory pages.
			//printf("MIGRATE MEMORY FROM %i TO %i\n", cluster, migration_cluster);
			//if (migrate_memory(migration_pid, cluster, migration_cluster) < 0) {
			//	printf("error in migrate_memory\n");
			//}
			pid_set_set_cluster(pid_set, migration_pid, migration_cluster);
            pid_set_set_cpu(pid_set, migration_pid, migration_cpu);
            cpu_pids[old_cpu]--;
            cpu_pids[migration_cpu]++;
			cluster_pids[cluster]--;
			cluster_pids[migration_cluster]++;
		}
	}
}

/* Start the next set of commands in queue. */
int run_commands(void)
{
	int fd, cluster, cpu, timeout;
	pid_t pid;
	cmd_t *cmd;
	/* Start all command queued for start at the current time. */
	while ((cmd = cmd_queue_front(cmd_queue)) != NULL && cmd->start
			<= run_clock) {
		cluster = get_optimal_cluster();
		cpu = get_optimal_cpu(cluster);
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Failed to fork process\n");
			return -1;
		} else if (pid == 0) {
			//			if (tmc_cpus_set_my_affinity(&cluster_sets[cluster]) < 0) {
			//				tmc_task_die("Failure in tmc_set_my_affinity()");
			//			}
			if (tmc_cpus_set_my_cpu(cpu) < 0) {
				tmc_task_die("Failure in tmc_set_my_cpu()");
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
			pid_set_insert(pid_set, pid, cpu, cluster, cmd->class);
			cluster_pids[cluster]++;
			cpu_pids[cpu]++;
		}
		/* Remove command from queue. */
		cmd_queue_dequeue(cmd_queue);
		printf("STARTED COMMAND ON CPU: %i\n", cpu);
		printf("CLUSTER COUNT WAS: %i\n", cluster_pids[cluster]);
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
	int cluster, cpu;
	pid_t pid;
	/* Await all terminated child processes. */
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		cluster = pid_set_get_cluster(pid_set, pid);
		cpu = pid_set_get_cpu(pid_set, pid);
		pid_set_remove(pid_set, pid);
		cluster_pids[cluster]--;
		cpu_pids[cpu]--;
	}
	/* Set flag if there are no more processes to wait for. */
	if (pid == -1 && all_started == 1) {
		all_terminated = 1;
	}
}

/* Periodically read PMC register values and calculate the miss rate. */
void *read_pmc(void *arg)
{
	int cpu;
	//int wr_miss, wr_cnt, drd_miss, drd_cnt;
	unsigned int wr_miss, drd_miss, bundles, data_cache_stalls;
	int miss_rate;
	struct timespec interval;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGPOLL);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

	cpu = (int) arg;
	interval.tv_sec = PMC_READ_INTERVAL;
	interval.tv_nsec = 0;
	/* Set thread affinity. */
	if (tmc_cpus_set_my_cpu(cpu) < 0) {
		fprintf(stderr, "Failed to start thread on cpu %i\n", cpu);
		tmc_task_die("failure in 'tmc_set_my_cpu'");
	}
	/* Setup PMC registers. */
	clear_counters();
	//setup_counters(LOCAL_WR_MISS, LOCAL_WR_CNT, LOCAL_DRD_MISS, LOCAL_DRD_CNT);
	setup_counters(LOCAL_WR_MISS, LOCAL_DRD_MISS, BUNDLES_RETIRED,
			DATA_CACHE_STALL);

	/* Periodically read registers, calculate miss rate and store value. */
	while (all_terminated == 0) {
//		printf("PMC READ ON CPU: %i THREAD: %u\n", tmc_cpus_get_my_current_cpu(), pthread_self());
		//read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
		read_counters(&wr_miss, &drd_miss, &bundles, &data_cache_stalls);
		clear_counters();
		//miss_rate = (wr_miss + drd_miss) / (wr_cnt + drd_cnt);
		miss_rate = ((wr_miss + drd_miss) * 1000) / (bundles);
		cpu_miss_rates[cpu] = miss_rate;
//		printf("CPU %i got miss rate %i\n", cpu, miss_rate);
//		if (miss_rate < 0) {
//			printf("MISS RATE NEGATIVE! wr_miss: %i, drd_miss: %i, bundles: %i\n", wr_miss, drd_miss, bundles);
//		}
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
	int cluster, optimal_cluster, count, best_count;
	int miss_rate, best_miss_rate;
	int limit;

	limit = 0;
	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
		limit += cluster_miss_rates[cluster];
	}
	limit = (limit/CPU_CLUSTERS) * MIGRATION_FACTOR;
 
	/* Find an active cluster with no running processes or select the cluster
	 * with least contention (lowest miss rate). */
	best_count = A_VERY_LARGE_NUMBER;
	best_miss_rate = A_VERY_LARGE_NUMBER;
	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
		if (cluster_pids[cluster] == 0) {
			return cluster;
		} else {
/*			miss_rate = cluster_miss_rates[cluster];
            if (miss_rate < best_miss_rate) {
                best_miss_rate = miss_rate;
                optimal_cluster = cluster;
            }
        }
    }
*/
			if (miss_rate < limit) {
				count = cluster_pids[cluster];
				if (count < best_count) {
					optimal_cluster = cluster;
					best_count = count;
				}
            } else {
                miss_rate = cluster_miss_rates[cluster];
                if (miss_rate < best_miss_rate) {
                    best_miss_rate = miss_rate;
                    optimal_cluster = cluster;
                }
		    }
        }
    }
//	for (cluster = 0; cluster < CPU_CLUSTERS; cluster++) {
//		if (cluster_pids[cluster] == 0) {
//			return cluster;
//		} else if (cluster_pids[cluster] < best_count) {
//			optimal_cluster = cluster;
//			best_count = cluster_pids[cluster];
//		}
//	}
	return optimal_cluster;
}

/* Returns the optimal cpu from the specified cluster. */
static int get_optimal_cpu(int cluster)
{
	int cpu, start;

	/* Set start to first cpu in cluster. */
	if (cluster == 0)
		start = 0;
	else if (cluster == 1)
		start = 4;
	else if (cluster == 2)
		start = 32;
	else if (cluster == 3)
		start = 36;
	/* Loop over cpus in cluster, return when an empty cpu is found. */
	for (cpu = start; cpu < start + 28; cpu += 4) {
		for (cpu = start; cpu < start + 4; cpu++) {
			if (cpu_pids[cpu] == 0) {
				return cpu;
			}
		}
	}

	return start;
}

/* Returns the accumulated miss rate for all CPUs in the specified CPU
 * cluster. */
static int get_cluster_miss_rate(int cluster)
{

	int miss_rates = 0;
	int start;
	if (cluster == 0)
		start = 0;
	else if (cluster == 1)
		start = 4;
	else if (cluster == 2)
		start = 32;
	else if (cluster == 3)
		start = 36;

	for (int i = 0; i < 4; i++) {
		miss_rates += cpu_miss_rates[start++];
		miss_rates += cpu_miss_rates[start++];
		miss_rates += cpu_miss_rates[start++];
		miss_rates += cpu_miss_rates[start++];
		start += 4;
	}
	return miss_rates;
}

/*
 * Migrates the memory of a process to a new cluster.
 */
static int migrate_memory(pid_t pid, int old_cluster, int new_cluster)
{

	nodemask_t fromnodes;
	nodemask_t tonodes;

	// Tileras numbering of memory controllers goes clockwise,
	// ours does not.
	if (old_cluster == 2) {
		old_cluster = 3;
	} else if (old_cluster == 3) {
		old_cluster = 2;
	}
	if (new_cluster == 2) {
		new_cluster = 3;
	} else if (new_cluster == 3) {
		new_cluster = 2;
	}

	nodemask_zero(&fromnodes);
	nodemask_zero(&tonodes);

	nodemask_set(&fromnodes, old_cluster);
	nodemask_set(&tonodes, new_cluster);

	return numa_migrate_pages(pid, &fromnodes, &tonodes);
}
