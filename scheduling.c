/* scheduling.c
 *
 * TODO: add description.
 * Scheduling routines. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "scheduling.h"

/* Helper prototypes. */
static int read_memprof_data(void);
static int get_optimal_cluster(void);
static int get_optimal_cpu(int cluster);

//static int migrate_memory(pid_t pid, int old_cluster, int new_cluster);

// DEBUG
void print_memprof_data(void)
{
	int cluster;
	for (cluster = 0; cluster < CPU_CLUSTER_COUNT; cluster++) {
		printf("CLUSTER: %i PIDS: %i MEMOPS: %lu\n", cluster,
				cluster_process_count[cluster], mc_rates[cluster]);
	}
}

/* Initialize scheduler. */
int init_scheduler(char *workload)
{
	int i;
	char *data_token, *value_token;
	char line_buffer[MEMPROF_BUFFER_SIZE];

	/* Get online CPUs and initialize CPU sets. */
	tmc_cpus_get_online_cpus(&online_set);

	tmc_cpus_clear(&cluster_sets[0]);
	tmc_cpus_clear(&cluster_sets[1]);
	tmc_cpus_clear(&cluster_sets[2]);
	tmc_cpus_clear(&cluster_sets[3]);

	tmc_cpus_grid_add_rect(&cluster_sets[0], 0, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[1], 4, 0, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[2], 0, 4, 4, 4);
	tmc_cpus_grid_add_rect(&cluster_sets[3], 4, 4, 4, 4);

	tmc_cpus_intersect_cpus(&cluster_sets[0], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[1], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[2], &online_set);
	tmc_cpus_intersect_cpus(&cluster_sets[3], &online_set);

	/* Set scheduler affinity (any online cpu). */
	if (tmc_cpus_set_my_affinity(&online_set)) {
		tmc_task_die("failure in set my affinity");
	}

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

	/* Initialize scheduling variables. */
	all_started = 0;
	all_terminated = 0;
	run_clock = 0;

	/* Initialize cluster process counters. */
	for (i = 0; i < CPU_CLUSTER_COUNT; i++) {
		cluster_process_count[i] = 0;
	}

	/* Initialize CPU process counters. */
	for (i = 0; i < CPU_COUNT; i++) {
		if (tmc_cpus_has_cpu(&online_set, i)) {
			cpu_process_count[i] = 0;
		} else {
			cpu_process_count[i] = -1;
		}
	}

	/* Initialize memory controller profiling. */
	memprof_file = fopen(MEMPROF_DATA_FILE, "r");
	if (memprof_file == NULL) {
		fprintf(stderr, "Failed to open memory controller profiling data\n");
		return -1;
	}
	while (fgets(line_buffer, MEMPROF_BUFFER_SIZE, memprof_file) != NULL) {
		data_token = strtok(line_buffer, " ");
		value_token = strtok(NULL, " ");
		if (data_token != NULL && value_token != NULL) {
			if (strcmp(data_token, "shim0_op_count:") == 0) {
				mc_ops[0] = strtoul(value_token, NULL, 0);
			} else if (strcmp(data_token, "shim1_op_count:") == 0) {
				mc_ops[1] = strtoul(value_token, NULL, 0);
			} else if (strcmp(data_token, "shim2_op_count:") == 0) {
				mc_ops[3] = strtoul(value_token, NULL, 0);
			} else if (strcmp(data_token, "shim3_op_count:") == 0) {
				mc_ops[2] = strtoul(value_token, NULL, 0);
			}
		}
	}

	return 0;
}

/* Perform scheduling tasks. */
void run_scheduler(void)
{
	pid_t pid;
	int i, migrate, best_cluster, worst_cluster, cpu;
	unsigned long int mc_rate_limit;

	/* Read memory operation statistics. */
	read_memprof_data();

	/* Calculate memory operation rate limit for mirgration to occur. */
	mc_rate_limit = 0;
	for (i = 0; i < CPU_CLUSTER_COUNT; i++) {
		mc_rate_limit += mc_rates[i];
	}
	mc_rate_limit /= CPU_CLUSTER_COUNT;
	mc_rate_limit *= MIGRATION_FACTOR;

	printf("MIGRATION LIMIT: %lu\n", mc_rate_limit);
	print_memprof_data();

	/* Check if process migration is needed. */
	migrate = 0;
	best_cluster = 0;
	worst_cluster = 0;
	for (i = 0; i < CPU_CLUSTER_COUNT; i++) {
		if (mc_rates[i] > mc_rate_limit) {
			migrate = 1;
		}
		if (mc_rates[i] < mc_rates[best_cluster]) {
			best_cluster = i;
		} else if (mc_rates[i] > mc_rates[worst_cluster]) {
			worst_cluster = i;
		}
	}

	if (migrate == 0 || best_cluster == worst_cluster
			|| cluster_process_count[worst_cluster] < 1) {
		return;
	} else {
		printf("MIGRATING CLUSTER %i TO %i\n", worst_cluster, best_cluster);

		pid = pid_set_get_minimum_pid(pid_set, worst_cluster);

		printf("GOT PID %i\n", pid);

		cpu = get_optimal_cpu(best_cluster);

		printf("GOT CPU %i\n", cpu);

		if (tmc_cpus_set_task_cpu(cpu, pid) < 0) {
			tmc_task_die("Failure in tmc_cpus_set_task_cpu()");
		}

		cluster_process_count[best_cluster]++;
		cluster_process_count[worst_cluster]--;

		cpu_process_count[cpu]++;
		cpu_process_count[pid_set_get_cpu(pid_set, pid)]--;

		pid_set_set_cluster(pid_set, pid, best_cluster);
		pid_set_set_cpu(pid_set, pid, cpu);

		printf("MIGRATED PID %i FROM CLUSTER %i TO %i\n", pid, worst_cluster,
				best_cluster);
		printf("NEW PID CLUSTER: %i\n", pid_set_get_cluster(pid_set, pid));
		printf("NEW PID CPU: %i\n", pid_set_get_cpu(pid_set, pid));

	}
}

/* Start the next set of commands in queue. */
int run_commands(void)
{
	int fd, cluster, cpu, timeout;
	int pid;
	cmd_t *cmd;
	/* Start all command queued for start at the current time. */
	while (cmd_queue_get_size(cmd_queue) > 0) {
		cmd = cmd_queue_front(cmd_queue);
		if (cmd->start > run_clock) {
			break;
		}
		cluster = get_optimal_cluster();
		cpu = get_optimal_cpu(cluster);
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Failed to fork process\n");
			return -1;
		} else if (pid > 0) {
			/* Add pid to cluster table. */
			pid_set_insert(pid_set, pid, cpu, cluster, cmd->class);
			cluster_process_count[cluster]++;
			cpu_process_count[cpu]++;
			/* Remove command from queue. */
			cmd_queue_dequeue(cmd_queue);
		} else {
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

			//			if (cmd->output_file != NULL) {
			//				fd = open(cmd->output_file, O_CREAT);
			//				if (fd == -1) {
			//					exit(EXIT_FAILURE);
			//				}
			//				dup2(fd, 1);
			//			}

			// SURPRESS ANOYING OUTPUT
			fd = open("/dev/null", O_CREAT);
			if (fd == -1) {
				exit(EXIT_FAILURE);
			}
			dup2(fd, 1);

			/* Execute program. */
			if (execv(cmd->cmd, cmd->argv) != 0) {
				exit(EXIT_FAILURE);
			}
		}
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
	pid_t pid;
	/* Await all terminated child processes. */
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		cluster_process_count[pid_set_get_cluster(pid_set, pid)]--;
		cpu_process_count[pid_set_get_cpu(pid_set, pid)]--;
		pid_set_remove(pid_set, pid);
	}
	/* Set flag if there are no more processes to wait for. */
	if (pid == -1 && all_started == 1) {
		all_terminated = 1;
	}
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

/**/
static int read_memprof_data(void)
{
	unsigned long int value;
	char *data_token, *value_token;
	char line_buffer[MEMPROF_BUFFER_SIZE];

	rewind(memprof_file);
	while (fgets(line_buffer, MEMPROF_BUFFER_SIZE, memprof_file) != NULL) {
		data_token = strtok(line_buffer, " ");
		value_token = strtok(NULL, " ");
		if (data_token != NULL && value_token != NULL) {
			if (strcmp(data_token, "shim0_op_count:") == 0) {
				value = strtoul(value_token, NULL, 0);
				mc_rates[0] = value - mc_ops[0];
				mc_ops[0] = value;
			} else if (strcmp(data_token, "shim1_op_count:") == 0) {
				value = strtoul(value_token, NULL, 0);
				mc_rates[1] = value - mc_ops[1];
				mc_ops[1] = value;
			} else if (strcmp(data_token, "shim2_op_count:") == 0) {
				value = strtoul(value_token, NULL, 0);
				mc_rates[3] = value - mc_ops[3];
				mc_ops[3] = value;
			} else if (strcmp(data_token, "shim3_op_count:") == 0) {
				value = strtoul(value_token, NULL, 0);
				mc_rates[2] = value - mc_ops[2];
				mc_ops[2] = value;
			}
		}
	}
	return 0;
}

/* Finds and returns the number of the cluster with least contention. */
static int get_optimal_cluster(void)
{
	int i, optimal_cluster;
	optimal_cluster = 0;
	for (i = 0; i < CPU_CLUSTER_COUNT; i++) {
		if (mc_rates[i] < mc_rates[optimal_cluster]) {
			optimal_cluster = i;
		}
	}
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
	for (cpu = start; cpu < start + 28; start += 4) {
		for (cpu = start; cpu < start + 4; cpu++) {
			if (cpu_process_count[cpu] == 0) {
				return cpu;
			}
		}
	}
	return start;
}

/* Migrates the memory of a process to a new cluster. */
//static int migrate_memory(pid_t pid, int old_cluster, int new_cluster)
//{
//	nodemask_t fromnodes;
//	nodemask_t tonodes;
//
//	// Tileras numbering of memory controllers goes clockwise,
//	// ours does not.
//	if (old_cluster == 2) {
//		old_cluster = 3;
//	} else if (old_cluster == 3) {
//		old_cluster = 2;
//	}
//	if (new_cluster == 2) {
//		new_cluster = 3;
//	} else if (new_cluster == 3) {
//		new_cluster = 2;
//	}
//
//	nodemask_zero(&fromnodes);
//	nodemask_zero(&tonodes);
//
//	nodemask_set(&fromnodes, old_cluster);
//	nodemask_set(&tonodes, new_cluster);
//
//	return numa_migrate_pages(pid, &fromnodes, &tonodes);
//}
