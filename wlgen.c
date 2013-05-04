#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int cmpfunc(const void *a, const void *b);

// starttime class dir cmd args
char *cmds[] = {"1 /opt/benchmarks/SPEC2006/benchspec/CPU2006/429.mcf/run/build_base_compsys.0000/ mcf ../../data/test/input/inp.in",
                "1 /opt/benchmarks/SPEC2006/benchspec/CPU2006/429.mcf/run/build_base_compsys.0000/ mcf ../../data/train/input/inp.in",
                "5 /opt/benchmarks/SPEC2006/benchspec/CPU2006/433.milc/run/build_base_compsys.0000/ milc < ../../data/test/input/su3imp3.in",
                "5 /opt/benchmarks/SPEC2006/benchspec/CPU2006/433.milc/run/build_base_compsys.0000/ milc < ../../data/train/input/su3imp.in",
                "0 /opt/benchmarks/SPEC2006/benchspec/CPU2006/450.soplex/run/build_base_compsys.0000/ soplex ../../data/test/input/test.mps",
                "0 /opt/benchmarks/SPEC2006/benchspec/CPU2006/450.soplex/run/build_base_compsys.0000/ soplex ../../data/train/input/train.mps",
                "0 /opt/benchmarks/SPEC2006/benchspec/CPU2006/470.lbm/run/build_base_compsys.0000/ lbm 20 myreference.dat 0 1 ../../data/test/input/100_100_130_cf_a.of",
                "0 /opt/benchmarks/SPEC2006/benchspec/CPU2006/470.lbm/run/build_base_compsys.0000/ lbm 300 myreference.dat 0 1 ../../data/train/input/100_100_130_cf_b.of"
               };

/*
 * Usage: ./wlgen <number_of_entries> <max_start_time> <output-workload-file>
 *
 * This creates a "random" workload with random start times from the given number
 * and the array of commands initialized above.
 */
int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("usage: /wlgen <number_of_entries> <max_start_time> <output-workload-file>\n");
        return -1;
    }
    int size_of_cmds = sizeof(cmds)/sizeof(cmds[0]);
    int num_of_entries = atoi(argv[1]);
    int max_start_time = atoi(argv[2]);
    char *workload = argv[3];

    srand(time(NULL));
    int times[num_of_entries];
    for(int i=0;i<num_of_entries;i++) {
        times[i] = rand() % max_start_time;
    }
    qsort(times, num_of_entries, sizeof(int), cmpfunc);

    // Open file in append-mode
    FILE *fp;
    fp=fopen(workload, "w");

    char cmd[512];
    for (int i=0;i<num_of_entries;i++) {
        sprintf(cmd, "%d %s\n", times[i], cmds[rand() % size_of_cmds]);
        fprintf(fp, cmd);
    }
    
    fclose(fp);

    return 0;
}

/*
 * Compare function to be used by qsort.
 */
int cmpfunc(const void *a, const void *b) {
   return (*(int*)a - *(int*)b);
}
