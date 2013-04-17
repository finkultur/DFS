/* pid_table_test.c */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pid_table.h"

static const int num_entries = 100;
static const int num_tiles = 64;

// Performs a test of the pid_table module:
int main(int argc, char *argv[])
{
    int n, pid, tile_num;
    pid_t pids[num_entries];
    pid_table table;

    srand(time(NULL ));
    // Create table:
    printf("creating table...");
    table = create_pid_table(4, 2);
    printf("done\n");
//    printf("table size: %zu\n", get_size(table));
//    printf("table count: %i\n", get_count(table));
//    print_table(table);
    // Insert elements
    for (n = 0; n < num_entries; n++)
    {
        pid = rand();
        pids[n] = pid;
        tile_num = rand() % num_tiles;
        printf("adding pid: %i tile_num: %i) ", pid, tile_num);
        if (add_pid(table, pid, tile_num) == 0)
        {
            printf("[ok]\n");
        }
        else
        {
            printf("[failed]\n");
        }
        //printf("table count: %i\n", get_count(table));
    }
//    print_table(table);
//    for (n = 0; n < num_entries; n++)
//    {
//        tile_num = rand() % 64;
//        printf("changing pid: %i tile_num: %i -> %i ", pids[n],
//                get_tile_num(table, pids[n]), tile_num);
//        if (set_tile_num(table, pids[n], tile_num) == 0)
//        {
//            printf("[ok]\n");
//        }
//        else
//        {
//            printf("[failed]\n");
//        }
//    }
//    print_table(table);
    // Remove elements:
    for (n = 0; n < num_entries; n++)
    {
        printf("removing pid: %i ", pids[n]);
        if (remove_pid(table, pids[n]) == 0)
        {
            printf("[ok]\n");
        }
        else
        {
            printf("[failed]\n");
        }
//        printf("table count: %i\n", get_count(table));
    }
//    print_table(table);
    for (n = 0; n < num_entries; n++)
    {
        pid = rand();
        pids[n] = pid;
        tile_num = rand() % num_tiles;
        printf("adding pid: %i tile_num: %i) ", pid, tile_num);
        if (add_pid(table, pid, tile_num) == 0)
        {
            printf("[ok]\n");
        }
        else
        {
            printf("[failed]\n");
        }

//        printf("table count: %i\n", get_count(table));
    }
//    print_table(table);
    printf("destroying table...");
    destroy_pid_table(table);
    printf("done\n");
    return 0;
}
