#ifndef TILEPOLL_H
#define TILEPOLL_H
struct tilepoll_struct {
    int my_tile;
    tile_table_t *table;
    cpu_set_t *cpus;
};
#endif

void *poll_my_pmcs(void *struct_with_all_args);
