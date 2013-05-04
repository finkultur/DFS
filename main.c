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
#include "cmd_list.h"

#define NUM_OF_CPUS 16

// RTS handlers:
void start_handler(int, siginfo_t*, void*);

// Functions that probably shouldn't be defined in main
int start_process(void);

// Global values:
int counter = 0;
cmd_list list;
cpu_set_t cpus;
int last_program_started = 0;
int childrens = 0;

/**
 * Main function.
 *
 * usage: ./main <workloadfile> [logfile]
 */
int main(int argc, char *argv[]) {

	// RTS actions:
    struct sigaction start_action;

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
    if (tmc_cpus_count(&cpus) < NUM_OF_CPUS) {
        tmc_task_die("Insufficient cpus available.");
    }

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
    
    // Start the first process(es) in file and setup timers and stuff.
    start_process();
    
    // While the last process hasn't started and children is still alive,
    // reap the dying children.
    int child_pid;
    while(childrens > 0 || last_program_started == 0) {
        child_pid = wait(NULL);
        if (child_pid > 0) {
            childrens--;
            printf("Number of unfinished children: %i", childrens);
        }
    }

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
        printf("Time %i: Start command: %s (not showing args)\n", 
               cmd->start_time, cmd->cmd);
        int pid = fork();
        if (pid < 0) {
            return 1; // Fork failed
        }
        else if (pid == 0) { // Child process
            /*   
            int mypid = getpid();
            int mycurcpu = tmc_cpus_get_my_current_cpu();
            printf("I am pid #%i and I am on physical tile #%i\n", 
                   mypid, mycurcpu);
            */
            chdir(cmd->dir);
            int status = execv(cmd->cmd, (char **)cmd->argv);
            printf("execvp failed with status %d\n", status);
            return 1;
        }
        else {
            childrens++;
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
    return 0;
}

