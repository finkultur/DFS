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
#include "parser.c"
#include "pid_table.h"

#define NUM_OF_CPUS 8
#define TABLE_SIZE 8

int start_process(void);
void start_handler(int);
void end_handler(int);
int get_tile(void);

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
    table = create_table(TABLE_SIZE);

    // Parse the file and set next to first entry in file.
    if ((list = create_cmd_list(argv[1])) == NULL) {
        printf("Failed to create command list from file: %s\n", argv[1]);
        return 1;
    }

    // Initialize signal handlers
    signal(SIGCHLD, end_handler);
    signal(SIGALRM, start_handler);

    // Start the first process(es) in file and setup timers and stuff.
    start_process();
    
    // Loop "forever", when the last job is done the program exits.
    while(1) {
        ;
    }

    return 0; // Unreachable code.
}

/*
 * Handles the SIGALRM sent by the timer.
 * Calls start_process()
 */
void start_handler(int sig) {
    start_process();
    signal(SIGALRM, start_handler);
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
        int tile_num = get_tile();
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
              
            // This is stupid.
            char fullpath[1024];
            getcwd(fullpath, 1024);
            strcat(fullpath, cmd->dir);
            chdir(fullpath);
            strcat(fullpath, cmd->cmd);

            int mypid = getpid();
            int mycurcpu = tmc_cpus_get_my_current_cpu();
            printf("I am pid #%i and I am on physical tile #%i, logical tile #%i\n", 
                   mypid, mycurcpu, tile_num);

            int status = execvp(fullpath, (char **)cmd->argv);
            printf("execvp failed with status %d\n", status);
            return 1;
        }
        else {
            // Add pid to table
            add_pid(table, pid, tile_num);
            // Print table for debugging purposes
            print_table(table);
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
        while (children_is_still_alive()) {
            ; // Loop until all process is done
        } 
        printf("My job here is done.\n");
        exit(0);
    }
    
    return 0;
}

/*
 * Handles SIGCHLD interrupts sent by dying children.
 */
void end_handler(int sig) {
    int pid;
    pid = wait(NULL);

    // Get what tile the pid ran on
    int tile_num = get_tile_num(table, pid);
    // Decrement the number of processes on that tile
    tileAlloc[tile_num]--;
    // Remove pid from pid_table
    remove_pid(table, pid);
    
    printf("pid: %d is done.\n", pid);
    signal(SIGCHLD, end_handler);
}

/*
 * Tries to get an empty tile.
 * If all is occupied, return 0 (performance wise, this is stupid...).
 * 
 * If no tile is free, it should be extended to return the tile with least 
 * contention.
 */
int get_tile(void) {
    for (int i=0;i<NUM_OF_CPUS;i++) {
        if (tileAlloc[i] == 0) {
            return i;
        }
    }
    return 0; 
}

/*
 * Checks if there are running child processes.
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
