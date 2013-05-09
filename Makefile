# Compilation / Linking:
CC = /opt/tilepro/bin/tile-cc
CFLAGS = -Wall -g -O0
#CFLAGS = -Wall -O3
LDFLAGS = -ltmc -lpthread -lrt -lnuma
SOURCES = main.c cmd_queue.c pid_set.c scheduling.c pmc.c
OBJECTS = $(addprefix $(BUILDDIR)/, main.o cmd_queue.o pid_set.o scheduling.o pmc.o)
EXECUTABLE = $(BUILDDIR)/dfs_nextgen
BUILDDIR = ./build
# Executing (using tile-monitor):
MONITOR = /opt/tilepro/bin/tile-monitor
MONARGS = --pci --tile 8x8 --hv-bin-dir /scratch/src/sys/hv --hvc /scratch/vmlinux-pci.hvc --upload /opt/tilepro/tile/usr/lib/libnuma.so.1 /usr/lib/libnuma.so.1 --here --mount-same /opt/benchmarks/SPEC2006/benchspec/CPU2006/
EXEARGS = workloads/wl_test_derp.txt #workloads/wl_test_May9_1620.txt #workloads/wl_test_1626_May4.txt 

all: $(OBJECTS) $(EXECUTABLE)
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

run: all
	$(MONITOR) $(MONARGS) -- $(EXECUTABLE) $(EXEARGS)  

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
