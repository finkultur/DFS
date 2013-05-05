#ifndef TILEPOLL_H
#define TILEPOLL_H
struct tilepoll_struct {
    int my_tile;
    float *my_miss_rate;
    cpu_set_t *cpus;
};
#endif

void *poll_my_pmcs(void *struct_with_all_args);
