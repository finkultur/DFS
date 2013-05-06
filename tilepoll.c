#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arch/cycle.h>

#include <tmc/cpus.h>
#include <tmc/task.h>

#include "perfcount.h"
#include "tile_table.h"
#include "tilepoll.h"

#define POLLING_INTERVAL 10

void *poll_my_pmcs(void *struct_with_all_args) {
    struct tilepoll_struct *data;
    data = (struct tilepoll_struct *) struct_with_all_args;

    int my_tile = data->my_tile;
    tile_table_t *table_ptr = data->table; 
    cpu_set_t *cpus_ptr = data->cpus;

    int wr_miss, wr_cnt, drd_miss, drd_cnt; 

    //free(data);

    // Set affinity   
    if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(cpus_ptr, my_tile)) < 0) {
        tmc_task_die("failure in 'tmc_set_my_cpu'");
    }
 
    // Setup counters
    clear_counters();
    setup_counters(LOCAL_WR_MISS, LOCAL_WR_CNT, LOCAL_DRD_MISS, LOCAL_DRD_CNT);

    // Loop forever, update miss rate table
    while(1) {
        //printf("Hi this is %i on cpu %i\n", my_tile, tmc_cpus_get_my_current_cpu());
        read_counters(&wr_miss, &wr_cnt, &drd_miss, &drd_cnt);
        clear_counters();
        // Push the data somewhere
        tile_table_set_miss_rate(table_ptr, ((float) (wr_miss+drd_miss)) / (wr_cnt+drd_cnt), my_tile);
        sleep(POLLING_INTERVAL);
    } 

}
