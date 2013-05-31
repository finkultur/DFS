#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int cmpfunc(const void *a, const void *b);

// starttime class dir cmd args
char *cmds[] = {"4 /opt/benchmarks/SPEC2006/benchspec/CPU2006/429.mcf/run/build_base_compsys.0000/ mcf ../../data/test/input/inp.in"
                ,"4 /opt/benchmarks/SPEC2006/benchspec/CPU2006/429.mcf/run/build_base_compsys.0000/ mcf ../../data/train/input/inp.in"
//                ,"1 /opt/benchmarks/SPEC2006/benchspec/CPU2006/433.milc/run/build_base_compsys.0000/ milc < ../../data/test/input/su3imp3.in"
//                ,"1 /opt/benchmarks/SPEC2006/benchspec/CPU2006/433.milc/run/build_base_compsys.0000/ milc < ../../data/train/input/su3imp.in"
                ,"3 /opt/benchmarks/SPEC2006/benchspec/CPU2006/450.soplex/run/build_base_compsys.0000/ soplex ../../data/test/input/test.mps"
                ,"3 /opt/benchmarks/SPEC2006/benchspec/CPU2006/450.soplex/run/build_base_compsys.0000/ soplex ../../data/train/input/train.mps"
                ,"3 /opt/benchmarks/SPEC2006/benchspec/CPU2006/470.lbm/run/build_base_compsys.0000/ lbm 20 myreference.dat 0 1 ../../data/test/input/100_100_130_cf_a.of"
//                ,"3 /opt/benchmarks/SPEC2006/benchspec/CPU2006/470.lbm/run/build_base_compsys.0000/ lbm 300 myreference.dat 0 1 ../../data/train/input/100_100_130_cf_b.of"
                , "1 /scratch/cBench/automotive_bitcount/src/ a.out 1125000" // auto_bitcount
                , "3 /scratch/cBench/consumer_jpeg_c/src/ a.out -dct int -progressive -opt -outfile output_large_encode.jpeg ../../consumer_jpeg_data/1.ppm"
                , "1 /scratch/cBench/network_dijkstra/src/ a.out ../../network_dijkstra_data/1.dat"
                , "4 /scratch/cBench/consumer_tiff2bw/src a.out ../../consumer_tiff_data/1.tif output_largebw.tif"
                , "4 /scratch/cBench/consumer_tiff2rgba/src/ a.out -c none ../../consumer_tiff_data/1.tif output_largergba.tif"
                , "4 /scratch/cBench/network_patricia/src/ a.out ../../network_patricia_data/1.udp" 
                , "5 /scratch/trashmem/ tmem"
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
