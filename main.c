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

#define NUM_OF_CPUS 16

//int startAllNow(struct cmdEntry *firstEntry, int numOfProcs);
//int parseFile(const char *filepath, struct cmdEntry *firstEntry);
//int parseLine(const char *line, struct cmdEntry *entry);

int start_process(void);
void start_handler(int);
void end_handler(int);

// Global values:
int counter = 0;
struct cmdEntry *firstEntry;
struct cmdEntry *next;


/**
 * Main function.
 *
 * usage: ./main <workloadfile> <logfile>
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

    firstEntry = malloc(sizeof(struct cmdEntry));
    parseFile(argv[1], firstEntry);

    signal(SIGCHLD, end_handler);
    signal(SIGALRM, start_handler);

    next = firstEntry;
    start_process();
    
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
        else if (pid == 0) { // Child process
            chdir(next->cwd);
            // Schedule somehow? :D
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
    printf("pid: %d is done.\n", pid);
    signal(SIGCHLD, end_handler);
}

// Starts all processes.
// Takes the first entry and proceedes until nextentry is NULL
/*
int startAllNow(struct cmdEntry *entryProcess, int numOfProcs) {
    
    struct cmdEntry *processToRun = entryProcess;
    int pid;

    for (int i=0; processToRun != NULL; i++) {

        pid = fork();
        if (pid < 0) {
            //tmc_task_die("fork() failed");
        }
        else if (pid == 0) { // Child process
            //printf("starting command #%u: %s,(sent from child)\n", i, processToRun->cmd); 
            sleep(processToRun->start_time);
            chdir(processToRun->cwd);
            
            // SCHEDULE HERE?
            //tmc_set_my_affinity(random(64)); // tile #i är antaligen upptagen

            int status = execvp(processToRun->cmd, (char **)processToRun->args);
            printf("execvp exited with status: %d\n", status);
            return 1;
        }
        else { // Parent process
            //wait(NULL);
        }
        processToRun = processToRun->nextEntry;
    }

    for (int i=0; i<numOfProcs; i++) {
        wait(NULL); //waitpid(pid, NULL, 0);
    }   
 
    return 0;
}
*/
