/*
 * This is supposed to be the DFS scheduler.
 *
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

//#include <tmc/cpus.h>
//#include <tmc/task.h>
//#include <tmc/udn.h>

#include "parser.c"

#define NUM_OF_CPUS 16

int startAllNow(struct cmdEntry *firstEntry, int numOfProcs);
//int parseFile(const char *filepath, struct cmdEntry *firstEntry);
//int parseLine(const char *line, struct cmdEntry *entry);

/**
 * Main function.
 *
 * usage: ./main <workloadfile> <logfile>
 */
int main(int argc, char *argv[]) {

    // Check command line arguments
    if (argc != 3) {
        printf("usage: %s <inputfile> <logfile>\n", argv[0]);
        return 1; // Error!
    }
    
    char *logfile = argv[2];
    printf("DFS scheduler initalized, writing output to %s\n", logfile);

    FILE *fp;
    fp = freopen(logfile, "a+", stdout);

    struct cmdEntry *entry = malloc(sizeof(struct cmdEntry));
    int numOfEntries = parseFile(argv[1], entry);
    startAllNow(entry, numOfEntries);

    return 0;
}

/* Starts all processes.
 * Takes the first entry and proceedes until nextentry is NULL
 */
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
        wait(NULL);
    }   
 
    return 0;
}
