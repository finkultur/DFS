/*
 * This is supposed to be the DFS scheduler.
 *
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sched.h>
#include <pthread.h>

// Tilera 
#include <tmc/cpus.h>
#include <tmc/task.h>
#include <tmc/udn.h>

// DFS 
#include "pid_table.h"
#include "cmd_list.h"
#include "sched_algs.h"
#include "migrate.h"
#include "perfcount.h"

#define NUM_OF_CPUS 8
#define TABLE_SIZE 8

// RTS handlers:
void start_handler(int, siginfo_t*, void*);
void end_handler(int, siginfo_t*, void*);

// Functions that probably shouldn't be defined in main
int start_process(void);
int children_is_still_alive(void);
void print_tileAlloc(void);

// Global values:
int counter = 0;
pid_table table;
int tileAlloc[NUM_OF_CPUS] = {0};
float wr_miss_rates[NUM_OF_CPUS] = {1.0};
float drd_miss_rates[NUM_OF_CPUS] = {1.0};
cmd_list list;
cpu_set_t cpus;
int last_program_started = 0;

/**
 * Main function.
 *
 * usage: ./main <workloadfile> [logfile]
 */
int main(int argc, char *argv[]) {

	// RTS actions:
    struct sigaction start_action;
    //struct sigaction end_action;

    // Save starting time
    long long int start_time = time(NULL);
    printf("Start time is: %lld\n", start_time);
    // Check command line arguments
    if (argc < 2 || argc > 3) {
        printf("usage: %s <inputfile> [logfile]\n", argv[0]);
        return 1; // Error!
    }
    else if (argc == 2) {
        printf("DFS scheduler initalized, writing output to stdout\n");
    }
    else if (argc == 3) { 
        char *logfile = argv[2];
        printf("DFS scheduler initalized, writing output to %s\n", logfile);
        freopen(logfile, "a+", stdout);
    }

    // Initialize cpu set
    if (tmc_cpus_get_my_affinity(&cpus) != 0) {
        tmc_task_die("Failure in 'tmc_cpus_get_my_affinity()'.");
    }
    printf("cpus_count is: %i\n", tmc_cpus_count(&cpus));
    if (tmc_cpus_count(&cpus) != NUM_OF_CPUS) {
//        tmc_task_die("Insufficient cpus available.");
        tmc_task_die("Got wrong number of cpus");
    }

    // Setup all performance counters on every initialized tile
    if (setup_all_counters(&cpus) != 0) {
        printf("setup_all_counters failed\n");
        return 1;
    }

    for (int i=0;i<NUM_OF_CPUS;i++) {
        tileAlloc[i] = 0;
    }
   
    // Initialize pid_table
    table = create_pid_table(TABLE_SIZE);
    // Parse the file and set next to first entry in file.
    if ((list = create_cmd_list(argv[1])) == NULL) {
        printf("Failed to create command list from file: %s\n", argv[1]);
        return 1;
    }
 
    // Define a struct containing data to be sent to thread
    struct poll_thread_struct *data = malloc(sizeof(struct poll_thread_struct));
    data->tileAlloc = tileAlloc;
    data->pidtable = table;
    data->cpus = &cpus;
    data->wr_miss_rates = wr_miss_rates;
    data->drd_miss_rates = drd_miss_rates;
 
    // Start the threads that polls the PMC registers
    pthread_t poll_pmcs_thread;
    pthread_create(&poll_pmcs_thread, NULL, poll_pmcs, (void*)data);

    // Init RTS for process start:
    start_action.sa_handler = NULL;
    start_action.sa_flags = SA_SIGINFO;
    start_action.sa_sigaction = start_handler;
    // Install handler (blocking SIGALRM/SIGSCHLD during handler execution):
    if (sigemptyset(&start_action.sa_mask) != 0
    		|| sigaddset(&start_action.sa_mask, SIGALRM) != 0
    		|| sigaddset(&start_action.sa_mask, SIGCHLD) != 0
    		|| sigaction(SIGALRM, &start_action, NULL) != 0) {
    	printf("Failed to setup handler for SIGALRM\n");
    }
    
    // Start the first process(es) in file and setup timers and stuff.
    start_process();
    
    // While the last process hasn't started and children is still alive,
    // reap the dying children.
    int child_pid;
    int child_tile_num;
    while(children_is_still_alive() || last_program_started == 0) {

        // Print reported miss rate
        /*for (int i=0;i<NUM_OF_CPUS;i++) {
            printf("tile %i's miss rate is %f\n", i, wr_miss_rates[i]);
        }*/

        // Reap child 
        child_pid = wait(NULL);
        if (child_pid > 0) {
            child_tile_num = get_tile_num(table, child_pid);
            tileAlloc[child_tile_num]--;
            remove_pid(table, child_pid);
        }
    }

    // This function (usually) prints performance counters & their miss rate
    get_tile_with_min_write_miss_rate(&cpus);

    // Print start, end and total time elapsed.
    long long int end_time = time(NULL);
    printf("Start time was: %lld\n", start_time);
    printf("End time was: %lld\n", end_time);
    long long int total_time;
    total_time = end_time - start_time;
    printf("Workload finished!\n");
    printf("Time elapsed: %lld\n", total_time);

    return 0;
}

/*
 * Handles the SIGALRM sent by the timer.
 * Calls start_process()
 */
void start_handler(int signo, siginfo_t *info, void *context) {
    start_process();
}

/*
 * Starts all processes with start_time less than or equal to the current
 * time/counter. Derps with the pid table, allocates a specific tile
 * to the process(es) and forks child process(es). Also keeps track of the
 * next set of program to start.
 */
int start_process() {

    struct itimerval timer;
    struct timeval timeout;
    cmd_entry cmd; 
 
    while ((cmd = get_first(list)) != NULL && cmd->start_time <= counter) {
        // Try to get an empty tile (or the tile with least contention)
        int tile_num = get_tile(&cpus, tileAlloc, wr_miss_rates);
        // Increment the number of processes running on that tile.
        tileAlloc[tile_num]++;
        print_tileAlloc();

        int pid = fork();
        if (pid < 0) {
            return 1; // Fork failed
        }
        else if (pid == 0) { // Child process
            if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(&cpus, tile_num)) < 0) {
                tmc_task_die("failure in 'tmc_set_my_cpu'");
            }
            
               
            int mypid = getpid();
            int mycurcpu = tmc_cpus_get_my_current_cpu();
            printf("Pid #%i: Physical #%i, Logical #%i. Processes on tile: %i, wr_miss_rate: %f\n", 
                   mypid, mycurcpu, tile_num, tileAlloc[tile_num], wr_miss_rates[tile_num]);
                        

            chdir(cmd->dir);
            int status = execv(cmd->cmd, (char **)cmd->argv);
            printf("execvp failed with status %d\n", status);
            return 1;
        }
        else {
            // Add pid to table
            add_pid(table, pid, tile_num);
            // Print table for debugging purposes
            //print_table(table);
        }
        remove_first(list);
    }
   
    if (cmd != NULL) { 
        timeout.tv_sec = (cmd->start_time)-counter;
        timeout.tv_usec = 0;
        timer.it_interval = timeout;
        timer.it_value = timeout;
        counter = cmd->start_time;
        setitimer(ITIMER_REAL, &timer, NULL);
    }
    else {
        last_program_started = 1;
    }
    // Disable timer. I got some weird segmentation fault with this:
    /*else { 
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        timer.it_interval = timeout;
        timer.it_value = timeout;
        counter = cmd->start_time;
        setitimer(ITIMER_REAL, &timer, NULL);
    }*/
    return 0;
}

/*
 * Prints the tileAlloc array
 */
void print_tileAlloc() {
    printf("Process allocation: ");
    for (int i=0;i<NUM_OF_CPUS;i++) {
        printf("%i:%i, ", i, tileAlloc[i]);
    }
    printf("\n");
}

/*
 * Checks if there are running child processes.
 * Returns 1 if any children is still alive.
 * Isn't it a syscall for this? 
 */
int children_is_still_alive() {
    for (int i=0;i<NUM_OF_CPUS;i++) {
        if (tileAlloc[i] != 0) {
            return 1;
        }
    }
    return 0;
}

