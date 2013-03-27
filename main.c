/*
 * This is supposed to be the DFS scheduler.
 *
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

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

// Global values:
int counter = 0;
struct cmdEntry *firstEntry;
struct cmdEntry *next;
pid_table table;

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
    
    // Loop "forever", when the last job is done the program returns anyway.
    while(1) {
        ;
    }

    return 0;
}

void start_handler(int sig) {
    start_process();
    signal(SIGALRM, start_handler);
}

int start_process() {

    struct itimerval timer;
    struct timeval timeout;
    // This might have to be in a separate function
    while (next != NULL && next->start_time <= counter) {
        int pid = fork();
        if (pid < 0) {
            return 1; // Fork failed
        }


        int tile_num = 2; // no it's not really on tile #2
        add_pid(table, pid, tile_num);

        if (pid == 0) { // Child process
            chdir(next->cwd);
            int status = execvp(next->cmd, (char **)next->args);
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

void end_handler(int sig) {
    int pid;
    pid = wait(NULL);

    // Print table (before deletion)
    print_table(table);
    // Remove pid
    remove_pid(table, pid);
    
    printf("pid: %d is done.\n", pid);
    signal(SIGCHLD, end_handler);
}
