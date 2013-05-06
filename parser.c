/*
 * Parser module for DFS
 */

#include <stdio.h>
#include <stdlib.h>

struct cmdEntry {
    int start_time;   // Start time (in seconds since program start)
    char *cmd;        // Command to run
    char **args;      // Argument to command. Must include cmd as [0]
    char *cwd;        // Directory to run command in
    int app_run_time; // Approximated run time (if "lonely")
    struct cmdEntry *nextEntry;
};

int parseFile(const char *filepath, struct cmdEntry *firstEntry);
int parseLine(const char *line, struct cmdEntry *entry);

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

    if ((line = fgets(line, 1024, file)) == NULL) {
        return numOfEntries; // No entries
    }
    parseLine(line, firstEntry);
    numOfEntries++; // Read one entry

    prev = firstEntry;
    while((line = fgets(line, 1024, file)) != NULL ) {
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
    char *time = malloc(sizeof(char)*8); //Just temporary, parsed to int later.

    char *cmd = malloc(sizeof(char)*256);
    char **args = malloc(sizeof(char)*512);
    char *cwd = malloc(sizeof(char)*256);

    if (line == NULL) {
        return 1; // ERROR
    }

    // Parse time
    for (i=0; line[index] != ',' && line[index] != '\0'; i++) {
        time[i] = line[index++];
    }
    index++;
    entry->start_time = atoi(time);

    // Parse command
    for (i=0; line[index] != ' ' && line[index] != '\0'; i++) {
       cmd[i] = line[index++];
    }
    // First argument passed to execvp must be the name of the executable:
    args[0] = cmd; 
    index++;
    cmd[i] = '\0';
    entry->cmd = cmd;

    // Parse arguments
    // i = argument number
    for (i=1; line[index] != ',' && line[index] != '\0' ; i++) {
        args[i] = malloc(sizeof(char)*16);
        for (int j=0; line[index] != ' ' && line[index] != ',' && line[index] != '\0'; j++) {
            args[i][j] = line[index++];
        }
    }
    index++;
    args[i] = '\0';
    entry->args = args;

    // Parse working directory
    for (i=0; line[index] != ',' && line[index] != '\0'; i++) {
       cwd[i] = line[index++];
    }
    index++;
    cwd[i] = '\0';
    entry->cwd = cwd;

    // Parse approximate run time
    for (i=0; line[index] != ',' && line[index] != '\0'; i++) {
        time[i] = line[index++];    
    }
    index++;
    time[i] = '\0';
    entry->app_run_time = atoi(time);

    return 0;
}

