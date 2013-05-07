/* main.c
 *
 * This is supposed to be the DFS scheduler. */

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "tile_table.h"

#include "cmd_queue.h"

#include "perfcount.h"

#include "sched_algs.h"

#define POLLING_INTERVAL 1
#define SCHEDULING_INTERVAL 2
#define NUM_OF_CLUSTERS 4
#define TOTAL_NUMBER_OF_TILES 64

static void *poll_my_pmcs(void *arg);
static void set_timer(timer_t *timer, int timeout);
static int run_commands(cmd_queue_t *queue, int time);

cpu_set_t all_cpus;
cpu_set_t *clusters[NUM_OF_CLUSTERS];

ctable_t *table;
int all_started, all_terminated;

int main(int argc, char *argv[])
{
	int current_time, timeout, signal;
	pid_t pid;
	cmd_queue_t *queue;
	sigset_t signal_mask;
	timer_t run_command_timer, scheduling_timer;
	struct sigevent run_commands_event, scheduling_event;
	long long int start_time, end_time;

    // Get start time
	start_time = time(NULL);

	// Check command line arguments
	if (argc != 2) {
		printf("usage: %s <inputfile>\n", argv[0]);
		return 1;
	}

	// Initialize cpu set
	if (tmc_cpus_get_my_affinity(&all_cpus) != 0) {
		tmc_task_die("failed to get cpu set\n");
	}

    // Save all online cpus to all_cpus
    tmc_cpus_get_online_cpus(&all_cpus);

    // Add 4 different grids with size 4x4 to different cpu_sets
    tmc_cpus_grid_add_rect(clusters[0], 0, 0, 4, 4);
    tmc_cpus_grid_add_rect(clusters[1], 4, 0, 4, 4);
    tmc_cpus_grid_add_rect(clusters[2], 0, 4, 4, 4);
    tmc_cpus_grid_add_rect(clusters[3], 4, 4, 4, 4);

    // Remove the tiles not online (e.g. reserved for I/O)
    tmc_cpus_intersect_cpus(clusters[0], &all_cpus);
    tmc_cpus_intersect_cpus(clusters[1], &all_cpus);
    tmc_cpus_intersect_cpus(clusters[2], &all_cpus);
    tmc_cpus_intersect_cpus(clusters[3], &all_cpus);

    // Initialize cluster table
	table = ctable_create();

	// Parse the file and set next to first entry in file.
	if ((queue = cmd_queue_create(argv[1])) == NULL) {
		printf("Failed to create command list from file: %s\n", argv[1]);
		return 1;
	}

    // Set signal masks
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGCHLD);
	sigaddset(&signal_mask, SIGALRM);
	sigaddset(&signal_mask, SIGPOLL);
	sigprocmask(SIG_BLOCK, &signal_mask, NULL);

	/* Create timers. */
	run_commands_event.sigev_notify = SIGEV_SIGNAL;
	run_commands_event.sigev_signo = SIGALRM;
	timer_create(CLOCK_REALTIME, &run_commands_event, &run_command_timer);

	scheduling_event.sigev_notify = SIGEV_SIGNAL;
	scheduling_event.sigev_signo = SIGPOLL;
	timer_create(CLOCK_REALTIME, &scheduling_event, &scheduling_timer);

	/* Set counters and flags. */
	current_time = 0;
	timeout = 0;

	all_started = 0;
	all_terminated = 0;

	// Start a polling thread for each cpu
    pthread_t polling_threads[TOTAL_NUMBER_OF_TILES];
	for (int i = 0; i < TOTAL_NUMBER_OF_TILES; i++) {
		pthread_create(&polling_threads[i], NULL, poll_my_pmcs, (void*) i);
	}

	/* Start the first process(es) from the command queue. 
	 * This will initialize the timer. */
	run_commands(queue, 0);

	/* Set timers. */
	if (cmd_queue_get_size(queue) > 0) {
		timeout = cmd_queue_front(queue)->start - current_time;
		set_timer(&run_command_timer, timeout);
		set_timer(&scheduling_timer, SCHEDULING_INTERVAL);

	} else {
		all_started = 1;
	}

	/* Listen for signals. */
	while (!all_terminated) {
		sigwait(&signal_mask, &signal);
		switch (signal) {

        // If SIGCHLD, reap a child and remove it from tile_table.
		case SIGCHLD:
			while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
				tile_table_remove(table, pid);
				printf("process %i terminated\n", pid);
			}
			if (pid == -1 && all_started) {
				all_terminated = 1;
			}
			break;

        // If SIGPOLL, Check for migration and (maybe) migrate.
		case SIGPOLL:
			printf("run scheduler\n");
			check_for_migration(table);
			printf("scheduling done\n");
			break;

        // If SIGALRM, it's time to start a new application!
		case SIGALRM:
			current_time = current_time + timeout;
			printf("starting commands, time: %i\n", current_time);
			run_commands(queue, current_time);

			if (cmd_queue_get_size(queue) > 0) {
				timeout = cmd_queue_front(queue)->start - current_time;
				set_timer(&run_command_timer, timeout);
			} else {
				all_started = 1;
				timer_delete(run_command_timer);
			}
			break;

		default:
			fprintf(stderr, "got invalid signal\n");
			break;
		}
	}

    // Get the end time and print the total elapsed time.
	end_time = time(NULL);
	printf("running time: %lld\n", end_time - start_time);

	return 0;
}

/*
 * Sets the specified timer to "pling" in timeout seconds.
 */
static void set_timer(timer_t *timer, int timeout)
{
	struct itimerspec value;
	value.it_interval.tv_sec = timeout;
	value.it_interval.tv_nsec = 0;
	value.it_value = value.it_interval;
	timer_settime(*timer, 0, &value, NULL);
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
static int run_commands(cmd_queue_t *queue, int time)
{
	int new_cluster;
	int fd;
	pid_t pid;
	cmd_t *cmd;

	while ((cmd = cmd_queue_front(queue)) != NULL && cmd->start <= time) {

		new_cluster = get_cluster_with_min_contention(table);

		pid = fork();
		if (pid < 0) {
			return -1;
		} else if (pid == 0) {
			if (tmc_cpus_set_my_affinity(clusters[new_cluster]) < 0) {
				tmc_task_die("failure in 'tmc_set_my_cpu'");
			}

			//printf("starting process: %i\n", getpid());

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
			ctable_insert(table, pid, new_cluster, cmd->class);
		}

		/* Remove command from queue. */
		cmd_queue_dequeue(queue);
	}
	return 0;
}

static void *poll_my_pmcs(void *arg)
{
	int my_tile = (int) arg;
	int wr_miss, wr_cnt, drd_miss, drd_cnt;
	float miss_rate;

    // Calculate row and column (to be sent to ctable) 
    int row = my_tile / 8;
    int col = my_tile % 8;

    // Check if my_tile exists in cpu_set (not occupied by I/O for example)
    if (tmc_cpus_has_cpu(&all_cpus, my_tile) > 0) {
        return NULL;
    }

    // Set affinity to that cpu	
	if (tmc_cpus_set_my_cpu(my_tile) < 0) {
		tmc_task_die("failure in 'tmc_set_my_cpu'");
	}

	// Setup counters
	clear_counters();
	setup_counters(LOCAL_WR_MISS, LOCAL_WR_CNT, LOCAL_DRD_MISS, LOCAL_DRD_CNT);

	// Loop until all applications are terminated, update miss rate table
	while (!all_terminated) {
		//printf("Hi this is %i on cpu %i\n", my_tile, tmc_cpus_get_my_current_cpu());
		read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
		clear_counters();
		// Push the data to the cluster table
		miss_rate = ((float) (wr_miss + drd_miss)) / (wr_cnt + drd_cnt);
		ctable_set_miss_rate(table, col, row, miss_rate);
		//printf("read PMC on tile: %i. miss rate: %f\n", my_tile, miss_rate);
		sleep(POLLING_INTERVAL);
	}
	return NULL;
}
