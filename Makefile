CC=gcc
TILECC=/opt/tilepro/bin/tile-cc
CFLAGS= -Wall -std=c99

main: main.o parser.o pid_table.o
	$(CC) $(CFLAGS) -o $@ main.o pid_table.o

tilera: main.o pid_table.o
	$(TILECC) $(CFLAGS) -c pid_table.c pid_table.o
	$(TILECC) $(CFLAGS) -c main.c main.o
	$(TILECC) $(CFLAGS) -ltmc -o main main.o pid_table.o
