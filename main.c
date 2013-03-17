/*
 * This is the DFS scheduler. IT ROCKS YO.
 */ 

#include <stdio.h>
#include <stdlib.h>

char testfile[] = "workload1";


initWorkLoad(char *filepath, struct *entry);
startAllNow(struct *entry);

int main() {

    struct *entry;

    initWorkLoad(testfile);
    startAllNow();

}

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

/*
 * Starts all processes.
 * Takes the first entry and proceedes until nextentry is NULL
 */
startAllNow(struct *entryProcess) {

    processToRun = entryProcess;

    for(int i=0; i<numberOfProcesses; i++) {

        derp = fork();
        if (derp == 0) {
            sleep(processToRun.starttid);
            tmc_set_my_affinity(i); // tile #i kansle är upptagen?
            execvp(processToRun.kommando);
        }
        else {
            //wait(NULL)
        }
        processToRun = processToRun.nextentry;

    }

}
