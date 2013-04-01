CC=gcc
TILECC=/opt/tilepro/bin/tile-cc
CFLAGS= -Wall -std=c99

tilera: main.o pid_table.o cmd_list.o
	$(TILECC) $(CFLAGS) -ltmc -o main main.o pid_table.o cmd_list.o

main.o: main.c
	$(TILECC) $(CFLAGS) -c main.c main.o

pid_table.o: pid_table.c pid_table.h
	$(TILECC) $(CFLAGS) -c pid_table.c pid_table.o

cmd_list.o: cmd_list.c cmd_list.h
	$(TILECC) $(CFLAGS) -c cmd_list.c cmd_list.o

