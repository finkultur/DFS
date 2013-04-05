CC=gcc
TILECC=/opt/tilepro/bin/tile-cc
CCFLAGS= -Wall #-std=c99
LNFLAGS= -ltmc

tilera: main.o pid_table.o cmd_list.o sched_algs.o perfcount.o
	$(TILECC) $(CCFLAGS) $(LNFLAGS) -o main main.o pid_table.o cmd_list.o sched_algs.o perfcount.o

main.o: main.c
	$(TILECC) $(CCFLAGS) -c main.c main.o

pid_table.o: pid_table.c pid_table.h
	$(TILECC) $(CCFLAGS) -c pid_table.c pid_table.o

cmd_list.o: cmd_list.c cmd_list.h
	$(TILECC) $(CCFLAGS) -c cmd_list.c cmd_list.o

sched_algs.o: sched_algs.c
	$(TILECC) $(CCFLAGS) -c sched_algs.c sched_algs.o

perfcount.o: perfcount.c perfcount.h
	$(TILECC) $(CCFLAGS) -c perfcount.c perfcount.o
