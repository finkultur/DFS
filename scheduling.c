/* scheduling.c
 *
 * Implementation of scheduling routines. */

#include <fcntl.h>
#include <numa.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "scheduling.h"

/* Flag values indicating when all commands from the workload have been started
 * and when all corresponding processes are terminated. */
int all_started = 0;
int all_terminated = 0;

/* Run time clock counter. */
int run_clock = 0;

/* Number of started processes. */
int start_count = 0;

/* Number of terminated processes (successfully). */
int end_count = 0;

/* Command queue. */
cmd_queue_t *queue;

/* Set of running processes (IDs). */
pid_set_t *set;

/* File for logging scheduler output. */
FILE *stdlog;

/* Memprof data input file. */
FILE *memprof_data;

/* Scheduling timers. */
timer_t command_timer, scheduling_timer;

/* All online CPUs. */
cpu_set_t online_cpus;

/* Start and end time for execution time measurement. */
struct timeval start_time, end_time;

/* Memory operation counter values for each memory controller. */
uint64_t mem_ops[NODE_COUNT];

/* Memory operation rates for each memory controller. */
uint64_t mem_rates[NODE_COUNT];

/* CPUs on each node. */
cpu_set_t node_cpus[NODE_COUNT];

/* Number of processes running on each node. */
int node_process_count[NODE_COUNT] = { 0 };

/* Number of processes running on each CPU. */
int cpu_process_count[CPU_COUNT] = { 0 };

/* Buffer used for parsing of data files. */
char buffer[BUFFER_SIZE];

/* Helper prototypes. */
static int set_timer(timer_t *timer, size_t timeout);
static int read_memprof_data(void);
static int get_optimal_node(void);
static int get_optimal_cpu(int node);

/* Initialize scheduler. */
int start_scheduler(char *workload, char *logfile)
{
	int node, cpu;
	char *high_word_token, *low_word_token, *data_token, *value_token;
	FILE *input;
	struct sigevent command_event, scheduling_event;
	uint32_t high_word, low_word;
	uint64_t cpu_mask;
	uint64_t node_masks[NODE_COUNT];
	/* Set start time. */
	gettimeofday(&start_time, NULL);
	/* Check for NUMA. */
	if (numa_available() < 0) {
		fprintf(stderr, "NUMA policy unavailable\n");
		return -1;
	}
	/* Open default or specified log file. */
	logfile = (logfile == NULL ? SCHEDULER_LOG : logfile);
	if ((stdlog = fopen(logfile, "a")) == NULL) {
		fprintf(stderr, "Failed to open log file %s\n", logfile);
		return -1;
	} else if (setvbuf(stdlog, NULL, _IOLBF, BUFFER_SIZE) != 0) {
		fprintf(stderr, "Failed to set log file buffer\n");
		return -1;
	}
	fprintf(stdlog, "Scheduler starting...\n");
	/* Initialize process set. */
	set = pid_set_create();
	if (set == NULL) {
		fprintf(stderr, "Failed to create process set\n");
		return -1;
	}
	/* Initialize command queue. */
	queue = cmd_queue_create(workload);
	if (queue == NULL) {
		fprintf(stderr, "Failed to create command queue\n");
		return -1;
	} else if (cmd_queue_get_size(queue) == 0) {
		fprintf(stderr, "Empty command queue\n");
		return -1;
	}
	fprintf(stdlog, "%i commands in queue\n", cmd_queue_get_size(queue));
	/* Get all online CPUs. */
	if (tmc_cpus_get_online_cpus(&online_cpus) < 0) {
		fprintf(stderr, "Failed to get online CPUs\n");
		return -1;
	}
	/* Read CPU maps and build CPU sets for each node. */
	for (node = 0; node < NODE_COUNT; node++) {
		cpu_mask = 1;
		tmc_cpus_clear(&node_cpus[node]);
		snprintf(buffer, BUFFER_SIZE, NODE_CPU_MAPS, node);
		if ((input = fopen(buffer, "r")) == NULL
				|| fgets(buffer, BUFFER_SIZE,input) == NULL
				|| (high_word_token = strtok(buffer, ",")) == NULL
				|| (low_word_token = strtok(NULL, ",")) == NULL) {
			fprintf(stderr, "Failed to read node %i CPU map\n", node);
			return -1;
		} else {
			fclose(input);
			high_word = strtoul(high_word_token, NULL, 16);
			low_word = strtoul(low_word_token, NULL, 16);
			node_masks[node] = high_word;
			node_masks[node] = node_masks[node] << 32;
			node_masks[node] += low_word;
		}
		for (cpu = 0; cpu < CPU_COUNT; cpu++) {
			if (node_masks[node] & cpu_mask) {
				tmc_cpus_add_cpu(&node_cpus[node], cpu);
			}
			cpu_mask = cpu_mask << 1;
		}
	}
	/* Initialize memory controller profiling. */
	memprof_data = fopen(MEMPROF_DATA_FILE, "r");
	if (memprof_data == NULL) {
		fprintf(stderr, "Failed to open memory controller profiling data\n");
		return -1;
	}
	while (fgets(buffer, BUFFER_SIZE, memprof_data) != NULL) {
		data_token = strtok(buffer, " ");
		value_token = strtok(NULL, " ");
		if (data_token != NULL && value_token != NULL) {
			if (strcmp(data_token, "shim0_op_count:") == 0) {
				mem_ops[0] = strtoull(value_token, NULL, 0);
			} else if (strcmp(data_token, "shim1_op_count:") == 0) {
				mem_ops[1] = strtoull(value_token, NULL, 0);
			} else if (strcmp(data_token, "shim2_op_count:") == 0) {
				mem_ops[2] = strtoull(value_token, NULL, 0);
			} else if (strcmp(data_token, "shim3_op_count:") == 0) {
				mem_ops[3] = strtoull(value_token, NULL, 0);
			}
		}
	}
	/* Create command start timer using SIGALRM. */
	command_event.sigev_notify = SIGEV_SIGNAL;
	command_event.sigev_signo = SIGALRM;
	if (timer_create(CLOCK_REALTIME, &command_event, &command_timer) != 0) {
		fprintf(stderr, "Failed to create command start timer\n");
		return -1;
	}
	/* Create scheduling interval timer using SIGPOLL. */
	scheduling_event.sigev_notify = SIGEV_SIGNAL;
	scheduling_event.sigev_signo = SIGPOLL;
	if (timer_create(CLOCK_REALTIME, &scheduling_event, &scheduling_timer) != 0) {
		fprintf(stderr, "Failed to create scheduling timer\n");
		return -1;
	}
	/* Start scheduling timer. */
	set_timer(&scheduling_timer, SCHEDULING_INTERVAL);
	/* Run commands. */
	if (cmd_queue_get_size(queue) > 0) {
		run_commands();
	} else {
		all_started = 1;
		all_terminated = 1;
	}
	return 0;
}

/* Stop scheduler and release opened resources. */
int stop_scheduler(void)
{
	long msec;
	if (all_started && all_terminated) {
		/* Set end time and print execution duration. */
		gettimeofday(&end_time, NULL);
		msec = (end_time.tv_sec - start_time.tv_sec) * 1000;
		msec += (end_time.tv_usec - start_time.tv_usec) / 1000;
		fprintf(stdlog, "Scheduler stopped\n");
        fprintf(stdlog, "%i commands started\n", start_count);
		fprintf(stdlog, "%i commands ran successfully\n", end_count);
		fprintf(stdlog, "Execution time: %lu,%lu seconds\n",
				msec / 1000, msec % 1000);
		/* Release allocated resources. */
		timer_delete(scheduling_timer);
		timer_delete(command_timer);
		cmd_queue_destroy(queue);
		pid_set_destroy(set);
		fclose(memprof_data);
		fclose(stdlog);
		return 1;
	} else {
		return 0;
	}
}

/* Perform scheduling tasks. */
void run_scheduler(void)
{
	int migrate, node, opt_cpu, opt_node, worst_node;
	process_t *process;
	uint64_t mem_rate_limit;
	/* Read memory operation statistics. */
	if (read_memprof_data() != 0) {
		fprintf(stdlog, "Failed to read memprof data\n");
		all_started = 1;
		all_terminated = 1;
		set_timer(&scheduling_timer, 0);
		return;
	}
	/* Calculate memory operation rate limit for mirgration to occur. */
	mem_rate_limit = 0;
	for (node = 0; node < NODE_COUNT; node++) {
		mem_rate_limit += mem_rates[node];
	}
	mem_rate_limit /= NODE_COUNT;
	mem_rate_limit *= MIGRATION_FACTOR;
	/* Check if process migration is needed. */
	migrate = 0;
	opt_node = 0;
	worst_node = 0;
	for (node = 0; node < NODE_COUNT; node++) {
		if (mem_rates[node] > mem_rate_limit) {
			migrate = 1;
		}
		if (mem_rates[node] < mem_rates[opt_node]) {
			opt_node = node;
		} else if (mem_rates[node] > mem_rates[worst_node]) {
			worst_node = node;
		}
	}
	/* Do migration if needed. */
	if (migrate && node_process_count[worst_node] > 1) {
		process = pid_set_get_maximum_process(set, worst_node);
		if (process == NULL) {
			fprintf(stdlog, "Migrate got a NULL process\n");
			return;
		}
		opt_cpu = get_optimal_cpu(opt_node);
		if (tmc_cpus_set_task_cpu(opt_cpu, process->pid) < 0) {
			fprintf(stdlog, "Migrate failed to set task cpu affinity\n");
			return;
		}
		node_process_count[worst_node]--;
		node_process_count[opt_node]++;
		cpu_process_count[process->cpu]--;
		cpu_process_count[opt_cpu]++;
		process->node = opt_node;
		process->cpu = opt_cpu;
		fprintf(stdlog, "Migrated pid %i from node %i to node %i\n",
				process->pid, worst_node, opt_node);
	}
}

/* Start the next set of commands in queue. */
void run_commands(void)
{
	int fd, node, cpu, timeout;
	pid_t pid;
	cmd_t *cmd;
	/* Start all command queued for start at the current time. */
	while (cmd_queue_get_size(queue) > 0 && cmd_queue_front(queue)->start
			<= run_clock) {
		cmd = cmd_queue_front(queue);
		node = get_optimal_node();
		cpu = get_optimal_cpu(node);

		pid = fork();
		if (pid < 0) {
			fprintf(stdlog, "Failed to fork process\n");
			cmd_queue_dequeue(queue);
		} else if (pid > 0) {
			/* Add process ID to set. */
            fprintf(stdlog, "Starting command: %s node: %i CPU: %i PID: %i\n",
                cmd->cmd, node, cpu, pid);
			pid_set_insert(set, pid, node, cpu, cmd->class);
			node_process_count[node]++;
			cpu_process_count[cpu]++;
			start_count++;
			/* Remove command from queue. */
			cmd_queue_dequeue(queue);
		} else {
			if (tmc_cpus_set_my_cpu(cpu) < 0) {
				fprintf(stdlog, "Failed to set task affinity. Node: %i CPU: %i\n"
						,node, cpu);
				exit(EXIT_FAILURE);
			}
			chdir(cmd->dir);
			/* Check redirection of stdin */
			if (cmd->input_file != NULL) {
				fd = open(cmd->input_file, O_RDONLY);
				if (fd == -1) {
					fprintf(stdlog, "Failed to redirect stdin\n");
					exit(EXIT_FAILURE);
				}
				dup2(fd, 0);
			}
			// Redirection of stdout
			if (cmd->output_file != NULL) {
				fd = open(cmd->output_file, O_CREAT);
				if (fd == -1) {
					fprintf(stdlog, "Failed to redirect stdout\n");
					exit(EXIT_FAILURE);
				}
				dup2(fd, 1);
			}
			/* Execute program. */
			if (execv(cmd->cmd, cmd->argv) != 0) {
				fprintf(stdlog, "Failed to execute command %s\n", cmd->cmd);
				exit(EXIT_FAILURE);
			}
		}
	}
	/* Set command timer of indicate all commands started. */
	if (cmd_queue_get_size(queue) > 0) {
		timeout = (cmd_queue_front(queue)->start - run_clock) * 1000;
		run_clock = cmd_queue_front(queue)->start;
		set_timer(&command_timer, timeout);
	} else {
		all_started = 1;
		set_timer(&command_timer, 0);
		fprintf(stdlog, "All commands started\n");
	}
}

/* Wait for terminated child processes. */
void await_processes(void)
{
    unsigned long long int start_time;
	int status;
	pid_t pid;
	process_t *process;
	/* Await all terminated child processes. */
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		process = pid_set_get_process(set, pid);
		if (process != NULL) {
            start_time = process->start_time;
			node_process_count[process->node]--;
			cpu_process_count[process->cpu]--;
			pid_set_remove(set, pid);
		} else {
			fprintf(stdlog, "Await process got a NULL process\n");
		}
		if (WIFEXITED(status)) {
			fprintf(stdlog, "Process %i terminated with status %i, It ran %llu secs.\n",
					pid, WEXITSTATUS(status), time(NULL)-start_time);
			end_count++;
		} else {
			fprintf(stdlog, "Process %i terminated abnormally\n", pid);
		}
	}
	/* Set flag if there are no more processes to wait for. */
	if (pid < 0 && all_started == 1) {
		all_terminated = 1;
		set_timer(&scheduling_timer, 0);
		fprintf(stdlog, "All processes terminated\n");
	}
}

/* Sets the timeout of the specified timer to the specified number of
 * milliseconds. Return 0 on success, otherwise -1. */
static int set_timer(timer_t *timer, size_t timeout)
{
	struct itimerspec value;
	value.it_interval.tv_sec = timeout / 1000;
	value.it_interval.tv_nsec = (timeout % 1000) * 1000000;
	value.it_value = value.it_interval;
	if (timer_settime(*timer, 0, &value, NULL) != 0) {
		fprintf(stdlog, "Failed to set timer\n");
		return -1;
	}
	return 0;
}

/* Reads memprof data and updates stored values. */
static int read_memprof_data(void)
{
	uint64_t value;
	char *data_token, *value_token;
	/* Read data file and parse values. */
	rewind(memprof_data);
	while (fgets(buffer, BUFFER_SIZE, memprof_data) != NULL) {
		data_token = strtok(buffer, " ");
		value_token = strtok(NULL, " ");
		if (data_token != NULL && value_token != NULL) {
			if (strcmp(data_token, "shim0_op_count:") == 0) {
				value = strtoull(value_token, NULL, 0);
				mem_rates[0] = value - mem_ops[0];
				mem_ops[0] = value;
			} else if (strcmp(data_token, "shim1_op_count:") == 0) {
				value = strtoull(value_token, NULL, 0);
				mem_rates[1] = value - mem_ops[1];
				mem_ops[1] = value;
			} else if (strcmp(data_token, "shim2_op_count:") == 0) {
				value = strtoull(value_token, NULL, 0);
				mem_rates[2] = value - mem_ops[2];
				mem_ops[2] = value;
			} else if (strcmp(data_token, "shim3_op_count:") == 0) {
				value = strtoull(value_token, NULL, 0);
				mem_rates[3] = value - mem_ops[3];
				mem_ops[3] = value;
			}
		}
	}
	return 0;
}

/* Finds and returns the number of the node with least contention. */
static int get_optimal_node(void)
{
	int node, opt_node;
	opt_node = 0;
	for (node = 0; node < NODE_COUNT; node++) {
		if (mem_rates[node] < mem_rates[opt_node]) {
			opt_node = node;
		}
	}
	return opt_node;
}

/* Returns the optimal CPU from the specified node. */
static int get_optimal_cpu(int node)
{
	int n, nmax, cpu, optimal_cpu;
	nmax = tmc_cpus_count(&node_cpus[node]);
	optimal_cpu = tmc_cpus_find_nth_cpu(&node_cpus[node], 0);
	for (n = 0; n < nmax; n++) {
		cpu = tmc_cpus_find_nth_cpu(&node_cpus[node], n);
		if (cpu_process_count[cpu] == 0) {
			return cpu;
		} else if (cpu_process_count[cpu] < cpu_process_count[optimal_cpu]) {
			optimal_cpu = cpu;
		}
	}
	return optimal_cpu;
}
