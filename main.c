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

// Tilera 
#include <tmc/cpus.h>
#include <tmc/task.h>
#include <tmc/udn.h>

// DFS 
#include "pid_table.h"
#include "cmd_list.h"
#include "sched_algs.h"

#define NUM_OF_CPUS 8
#define TABLE_SIZE 8

// RTS handlers:
void start_handler(int, siginfo_t*, void*);
void end_handler(int, siginfo_t*, void*);

int start_process(void);
int children_is_still_alive(void);

// Global values:
int counter = 0;
pid_table table;
int tileAlloc[NUM_OF_CPUS];
cmd_list list;
cpu_set_t cpus;

/**
 * Main function.
 *
 * usage: ./main <workloadfile> [logfile]
 */
int main(int argc, char *argv[]) {

	// RTS actions:
    struct sigaction start_action;
    struct sigaction end_action;

    // Save starting time
    long long int start_time = time(NULL);
    printf("Start time is: %lld\n", start_time);

    // Initialize tileAlloc to zeros, this shouldn't be necessary (?)
    for (int i=0;i<NUM_OF_CPUS;i++) {
        tileAlloc[i] = 0;
    }

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
    if (tmc_cpus_count(&cpus) < NUM_OF_CPUS) {
        tmc_task_die("Insufficient cpus available.");
    }

    // Initialize pid_table
    table = create_pid_table(TABLE_SIZE);

    // Parse the file and set next to first entry in file.
    if ((list = create_cmd_list(argv[1])) == NULL) {
        printf("Failed to create command list from file: %s\n", argv[1]);
        return 1;
    }

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

    // Init RTS for process termination:
    end_action.sa_handler = NULL;
    end_action.sa_flags = SA_SIGINFO;
    end_action.sa_sigaction = end_handler;
    // Install handler (blocking SIGCHLD/SIGALRM during handler execution):
    if (sigemptyset(&end_action.sa_mask) != 0
    	|| sigaddset(&end_action.sa_mask, SIGCHLD) != 0
    	|| sigaddset(&end_action.sa_mask, SIGALRM) != 0
    	|| sigaction(SIGCHLD, &end_action, NULL) != 0) {
    	printf("Failed to setup handler for SIGCHLD\n");
    }

    // Start the first process(es) in file and setup timers and stuff.
    start_process();
    
    // Loop "forever", when the last job is done the program exits.
    while(children_is_still_alive()) {
        ;
    }

    // Print time
    long long int end_time = time(NULL);
    printf("Start time is: %lld\n", start_time);
    printf("End time is: %lld\n", end_time);

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
 * Handles SIGCHLD interrupts sent by dying children.
 */
void end_handler(int signo, siginfo_t *info, void *context)
{
    int pid;
    pid = wait(NULL);

    // Get what tile the pid ran on
    int tile_num = get_tile_num(table, pid);
    // Decrement the number of processes on that tile
    tileAlloc[tile_num]--;
    // Remove pid from pid_table
    remove_pid(table, pid);
    // Printing from signal handlers is stupid:
    //printf("pid: %d is done.\n", pid);
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
        int tile_num = get_tile(&cpus, tileAlloc);
        // Increment the number of processes running on that tile.
        tileAlloc[tile_num]++;

        int pid = fork();
        if (pid < 0) {
            return 1; // Fork failed
        }
        else if (pid == 0) { // Child process
            
            if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(&cpus, tile_num)) < 0) {
                tmc_task_die("failure in 'tmc_set_my_cpu'");
            }
            
            /*   
            int mypid = getpid();
            int mycurcpu = tmc_cpus_get_my_current_cpu();
            printf("I am pid #%i and I am on physical tile #%i, logical tile #%i\n", 
                   mypid, mycurcpu, tile_num);
            */
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
