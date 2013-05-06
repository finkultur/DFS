# Compilation / Linking:
CC = /opt/tilepro/bin/tile-cc
#CFLAGS = -Wall -g -O0
CFLAGS = -Wall -O3
LDFLAGS = -ltmc -lpthread -lrt
SOURCES = main.c cmd_queue.c tile_table.c pid_set.c perfcount.c sched_algs.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = dfs.bin
# Executing (using tile-monitor):
MONITOR = /opt/tilepro/bin/tile-monitor
MONARGS = --pci --tile 4x1 --hv-bin-dir /scratch/src/sys/hv --here --mount-same /opt/benchmarks/SPEC2006/benchspec/CPU2006/
EXEARGS = workload.txt 4

all: $(OBJECTS) $(EXECUTABLE)
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

run: all
	$(MONITOR) $(MONARGS) -- $(EXECUTABLE) $(EXEARGS)  

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)  

%.o: %.c
	$(CC) -c $(CFLAGS) $<
