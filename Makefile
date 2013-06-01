# Compilation / Linking:
CC = /opt/tilepro/bin/tile-cc
CFLAGS = -Wall -Os
LDFLAGS = -lrt
SOURCES = main.c cmd_queue.c scheduling.c
OBJECTS = $(addprefix $(BUILDDIR)/, main.o cmd_queue.o scheduling.o)
EXECUTABLE = $(BUILDDIR)/cfs
BUILDDIR = ./build
# Executing (using tile-monitor):
EXEARGS = workloads/wl_107.txt logfile_wl107_cfs_run.txt
MONITOR = /opt/tilepro/bin/tile-monitor
MONARGS = \
	--pci \
	--mount-same /opt/benchmarks/SPEC2006/benchspec/CPU2006/ \
	--mount-same /scratch/trashmem/ \
	--mount-same /scratch/cBench/ \
	--here

MCSTAT = mcstat -c

all: clean builddir $(OBJECTS) $(EXECUTABLE)
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

run: all
	$(MONITOR) $(MONARGS) -- $(MCSTAT) $(EXECUTABLE) $(EXEARGS)  

run_grid: all
	env \
	TILERA_IDE_PORT=tilera:58744 \
	TILERA_IDE_TEE=1 \
	$(MONITOR) $(MONARGS) --debug-on-crash -- $(EXECUTABLE) $(EXEARGS)  

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

builddir:
	mkdir -p ./build
