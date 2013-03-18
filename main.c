/*
 * This is the DFS scheduler. IT ROCKS YO.
 *
 * THIS FILE SHOULD BE SPLIT INTO MULTIPLE FILES
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct cmdEntry {
    int start_time;
    char *cmd;
    char *cwd;
    struct cmdEntry *nextEntry;
};

//initWorkLoad(char *filepath, struct *entry);
//startAllNow(struct *entry);
int parseFile(const char *filepath, struct cmdEntry *firstEntry);
int parseLine(const char *line, struct cmdEntry *entry);


int main(int argc, char *argv[]) {

    // Check command line arguments
    if (argc != 2) {
        printf("usage: %s <inputfile>\n", argv[0]);
        return 1; // Error!
    }
    printf("DFS scheduler initalized.\n");

    struct cmdEntry *entry = malloc(sizeof(struct cmdEntry));
    int numOfEntries = parseFile(argv[1], entry);
    /*
    // Testing parseFile:
    for(int i=0; i<numOfEntries; i++) {
        printf("Entry: %u, %s, %s", entry->start_time, entry->cmd, entry->cwd);
        entry = entry->nextEntry;
    } 
    */
    //initWorkLoad(testfile);
    startAllNow(entry);

}
/*
initWorkLoad(char *filepath, struct *entry) {

    entry = malloca en ny struct

    Öppna filen
    För varje rad:
        struct.starttid = starttid från rad
        struct.kommando = kommando från rad
        struct.cwd = cwd från rad
        spara cwd
        if (En till rad) {
            struct.nextentry = en ny struct
        }
        else {
            struct.nextentry = NULL;
        }

}
*/
/* Starts all processes.
 * Takes the first entry and proceedes until nextentry is NULL
 */
int startAllNow(struct cmdEntry *entryProcess) {

    struct cmdEntry *processToRun = entryProcess;
    int derp;
    char *herp[3];
    herp[0] = "echo"; // derp derp dep
    herp[1] = "5"; // just 
    herp[2] = NULL; // testing 

    chdir("/home/faj");

    for(int i=0; processToRun != NULL; i++) {

        derp = fork();
        if (derp == 0) {
            printf("starting command #%u: %s ,(sent from child)\n", i, processToRun->cmd); 
            //sleep(processToRun.starttid);
            //tmc_set_my_affinity(i); // tile #i kansle är upptagen?
            int status = execvp(herp[0], (char **)&herp);
            printf("execvp exit with status: %d\n", status);
            exit(0);
        }
        else {
            wait(NULL);
        }
        processToRun = processToRun->nextEntry;

    }

}

/*
 * Parses a file. 
 * filepath: String containing filepath
 * firstEntry: Where to store the first command entry
 *
 * Returns the number of entries in file.
 */
int parseFile(const char *filepath, struct cmdEntry *firstEntry) {

    int numOfEntries = 0;
    struct cmdEntry *prev = malloc(sizeof(struct cmdEntry));
    struct cmdEntry *next;
    char *line = malloc(sizeof(char)*1024);
    FILE *file;
    file = fopen(filepath, "r"); // Open file read-only

    if ((line = fgets(line, 512, file)) == NULL) {
        return numOfEntries; // No entries
    }
    parseLine(line, firstEntry);
    numOfEntries++; // Read one entry

    prev = firstEntry;
    while((line = fgets(line, 512, file)) != NULL ) {
        next = malloc(sizeof(struct cmdEntry));
        parseLine(line, next);
        prev->nextEntry = next;
        prev = next;
        numOfEntries++;
    }
    
    return numOfEntries;
}

/*
 * Parses a line containing an command entry.
 * line: line to parse
 * entry: where to save the parsed entry
 *
 * Returns 0 if successful, 1 otherwise.
 */
int parseLine(const char *line, struct cmdEntry *entry) {
    
    int i;
    int index = 0; // index of line
    char *time = malloc(sizeof(char)*8); //Just temporary
    
    int starttime;
    char *cmd = malloc(sizeof(char)*256);
    char *cwd = malloc(sizeof(char)*256);
    
    if (line == NULL) {
        return 1; // ERROR
    }

    // Parse time
    for (i=0; line[index] != ',' && line[index] != '\0'; i++) {
        time[i] = line[index++];
    }
    index++;
    starttime = atoi(time);
    entry->start_time = starttime;

    // Parse command
    for (i=0; line[index] != ',' && line[index] != '\0'; i++) {
       cmd[i] = line[index++]; 
    }
    index++;
    cmd[i] = '\0';
    entry->cmd = cmd;

    // Parse working directory
    for (i=0; line[index] != ',' && line[index] != '\0'; i++) {
       cwd[i] = line[index++]; 
    }
    cwd[i] = '\0';
    entry->cwd = cwd;
    return 0; 
}





















