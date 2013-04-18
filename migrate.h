#ifndef MIGRATE_H
#define MIGRATE_H
// Struct to be sent to thread that polls pmcs
struct poll_thread_struct {
    pid_table pidtable;
    int *tileAlloc;
    float *wr_miss_rates;
    float *drd_miss_rates;
    cpu_set_t *cpus;
};
#endif

// Function prototypes
void *poll_pmcs(void *struct_with_args);
void cool_down_tile(int tile_num);
void migrate_process(int pid, int new_tile);
void print_wr_miss_rates(void);
int get_first_pid_from_tile(int tilenum); // REMOVE
