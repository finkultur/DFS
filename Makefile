# Compilation / Linking:
CC = /opt/tilepro/bin/tile-cc
CFLAGS = -Wall -Os
LDFLAGS = -ltmc -lrt -lnuma
SOURCES = main.c cmd_queue.c pid_set.c scheduling.c
OBJECTS = $(addprefix $(BUILDDIR)/, main.o cmd_queue.o pid_set.o scheduling.o)
EXECUTABLE = $(BUILDDIR)/dfs
BUILDDIR = ./build
# Executing (using tile-monitor):
EXEARGS = workloads/workload.txt
MONITOR = /opt/tilepro/bin/tile-monitor
MONARGS = \
	--pci \
	--hvc /scratch/vmlinux-pci.hvc \
	--upload /opt/tilepro/tile/usr/lib/libnuma.so.1 /usr/lib/libnuma.so.1 \
	--mount-same /opt/benchmarks/SPEC2006/benchspec/CPU2006/ \
	--here

all: clean builddir $(OBJECTS) $(EXECUTABLE)
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

run: all
	$(MONITOR) $(MONARGS) -- $(EXECUTABLE) $(EXEARGS)  

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