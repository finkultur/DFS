#ifndef MIGRATE_H
#define MIGRATE_H
// Struct to be sent to thread that polls pmcs
struct poll_thread_struct {
    proc_table proctable;
    float *wr_miss_rates;
    float *drd_miss_rates;
    cpu_set_t *cpus;
};
#endif

// Function prototypes
void *poll_pmcs(void *struct_with_args);
void check_for_possible_migration(proc_table table);
void migrate_smallest(proc_table table, int tilenum);
void cool_down_tile(proc_table table, int tile_num, int how_much);
void chill_it(proc_table table, int tilenum);
void migrate_process(proc_table table, int pid, int new_tile);
