# Compilation / Linking:
CC = /opt/tilepro/bin/tile-cc
CFLAGS = -Wall -g -O0
#CFLAGS = -Wall -O3
LDFLAGS = -ltmc -lrt -lnuma
SOURCES = main.c cmd_queue.c pid_set.c scheduling.c
OBJECTS = $(addprefix $(BUILDDIR)/, main.o cmd_queue.o pid_set.o scheduling.o)
EXECUTABLE = $(BUILDDIR)/dfs_nextgen
BUILDDIR = ./build
# Executing (using tile-monitor):
MONITOR = /opt/tilepro/bin/tile-monitor
MONARGS = \
	--pci \
	--tile 8x8 \
	--hv-bin-dir /scratch/src/sys/hv \
	--hvc /scratch/vmlinux-pci.hvc \
	--upload /opt/tilepro/tile/usr/lib/libnuma.so.1 /usr/lib/libnuma.so.1 \
	--mount-same /opt/benchmarks/SPEC2006/benchspec/CPU2006/ \
	--mount-same /scratch/cBench/automotive_bitcount/src/ \
	--here \
	--quit \
#	-- cat /proc/692/numa_maps
#   --hvx LD_CACHE_HASH=ro \
#   --hvx dataplane=1-63 \

#EXEARGS = workloads/wl_yeah.txt # ~ 560s
#EXEARGS = workloads/wl_yeahX2.txt # ~ ?s
#EXEARGS = workloads/wl_test_derp.txt # ~ 85s
#EXEARGS = workloads/wl_test.txt # ~ 124s
EXEARGS = workloads/wl_May15_1.txt # ~ ? s

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
