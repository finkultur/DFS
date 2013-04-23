CC=gcc
TILECC=/opt/tilepro/bin/tile-cc
CCFLAGS= -Wall #-std=c99
LNFLAGS= -ltmc -pthread

tilera: main.o proc_table.o pid_table.o tile_table.o cmd_list.o sched_algs.o perfcount.o migrate.o
	$(TILECC) $(CCFLAGS) $(LNFLAGS) -o main main.o proc_table.o tile_table.o pid_table.o cmd_list.o sched_algs.o perfcount.o migrate.o

main.o: main.c
	$(TILECC) $(CCFLAGS) -c main.c main.o

pid_table.o: pid_table.c pid_table.h
	$(TILECC) $(CCFLAGS) -c pid_table.c pid_table.o

tile_table.o: tile_table.c tile_table.h
	$(TILECC) $(CCFLAGS) -c tile_table.c tile_table.o

proc_table.o: proc_table.c proc_table.h pid_table.o tile_table.o
	$(TILECC) $(CCFLAGS) -c proc_table.c proc_table.o

cmd_list.o: cmd_list.c cmd_list.h
	$(TILECC) $(CCFLAGS) -c cmd_list.c cmd_list.o

sched_algs.o: sched_algs.c
	$(TILECC) $(CCFLAGS) -c sched_algs.c sched_algs.o

perfcount.o: perfcount.c perfcount.h
	$(TILECC) $(CCFLAGS) -c perfcount.c perfcount.o

migrate.o: migrate.c migrate.h
	$(TILECC) $(CCFLAGS) -c migrate.c migrate.o
