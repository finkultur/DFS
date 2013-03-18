/*
 * This is suppose to be the DFS scheduler.
 *
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "parser.c"

int startAllNow(struct cmdEntry *firstEntry);
//int parseFile(const char *filepath, struct cmdEntry *firstEntry);
//int parseLine(const char *line, struct cmdEntry *entry);

/**
 * Main function.
 *
 * usage: ./main <workloadfile>
 */
int main(int argc, char *argv[]) {

    // Check command line arguments
    if (argc != 2) {
        printf("usage: %s <inputfile>\n", argv[0]);
        return 1; // Error!
    }
    printf("DFS scheduler initalized.\n");

    struct cmdEntry *entry = malloc(sizeof(struct cmdEntry));
    /*int numOfEntries = */
    parseFile(argv[1], entry);
    startAllNow(entry);

    return 0;
}

/* Starts all processes.
 * Takes the first entry and proceedes until nextentry is NULL
 */
int startAllNow(struct cmdEntry *entryProcess) {
    
    struct cmdEntry *processToRun = entryProcess;
    int pid;

    for(int i=0; processToRun != NULL; i++) {

        pid = fork();
        if (pid == 0) { // Child process
            //printf("starting command #%u: %s,(sent from child)\n", i, processToRun->cmd); 
            sleep(processToRun->start_time);
            chdir(processToRun->cwd);
            //tmc_set_my_affinity(i); // tile #i kansle är upptagen?
            int status = execvp(processToRun->cmd, (char **)processToRun->args);
            printf("execvp exit with status: %d\n", status);
            return 1;
        }
        else { // Parent process
            //wait(NULL);
        }
        processToRun = processToRun->nextEntry;

    }
    return 0;
}
