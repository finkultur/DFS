/*
 * Example program, sleeps for the specified number of seconds.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("No time supplied or wrong number of args\n");
        return -1;
    }
    int time = atoi(argv[1]);
    int pid = getpid();
    printf("Hi, this is pid: %i\n", pid); 
    sleep(time);
    printf("pid: %i ran for %i seconds\n", pid, time);
    
    return 0;
    
}
