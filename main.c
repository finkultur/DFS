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

//#include <tmc/cpus.h>
//#include <tmc/task.h>
//#include <tmc/udn.h>

#include "parser.c"
#include "pid_table.h"

#define NUM_OF_CPUS 16
#define TABLE_SIZE 8

//int parseFile(const char *filepath, struct cmdEntry *firstEntry);
//int parseLine(const char *line, struct cmdEntry *entry);

int start_process(void);
void start_handler(int);
void end_handler(int);
int get_tile(void);

// Global values:
int counter = 0;
struct cmdEntry *firstEntry;
struct cmdEntry *next;
pid_table table;
int tileAlloc[NUM_OF_CPUS];

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

    // Initialize pid_table
    table = create_table(TABLE_SIZE);

    // Parse the file and set firstEntry
    firstEntry = malloc(sizeof(struct cmdEntry));
    parseFile(argv[1], firstEntry);

    // Initialize signal handlers
    signal(SIGCHLD, end_handler);
    signal(SIGALRM, start_handler);

    // Do the first entry in file to setup timers and stuff.
    next = firstEntry;
    start_process();
    
    // Loop "forever", when the last job is done the program returns.
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
    // This might have to be in a separate function
    while (next != NULL && next->start_time <= counter) {
        int pid = fork();
        if (pid < 0) {
            return 1; // Fork failed
        }
        else if (pid > 0) { // Parent process
            // Try to get an empty tile
            int tile_num = get_tile();
            // Increment the number of processes running on that tile.
            tileAlloc[tile_num]++;
            // Add pid to table
            add_pid(table, pid, tile_num);

            // Print table for debugging purposes
            print_table(table);
        }
        else if (pid == 0) { // Child process

            // This is stupid.
            char fullpath[1024];
            fullpath[0] = '\0';
            strcat(fullpath, next->cwd);
            strcat(fullpath, next->cmd);
            chdir(next->cwd);

            int status = execvp(fullpath, (char **)next->args);
            printf("execvp failed with status %d\n", status);
            return 1;
        }
        next = next->nextEntry;
    }
   
    if (next != NULL) { 
        timeout.tv_sec = (next->start_time)-counter;
        timeout.tv_usec = 0;
        timer.it_interval = timeout;
        timer.it_value = timeout;
        counter = next->start_time;
        setitimer(ITIMER_REAL, &timer, NULL);
    }
    else {
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
