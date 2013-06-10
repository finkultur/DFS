DFS
This is the DFS-scheduler. It's a user-space contention-aware scheduler for Linux adapted for the Tilera TILEPro64-processor.

To run a workload: 
- Modify the EXEARGS-line in the Makefile
- make run

To generate a new workload:
- gcc -std=c99 -o wlgen wlgen.c
- ./wlgen <number-of-entries> <max-start-time> <output-workload-file>

