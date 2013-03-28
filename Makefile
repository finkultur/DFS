CC=gcc
TILECC=/opt/tilepro/bin/tile-cc
CFLAGS= -Wall -std=c99

main: main.o parser.o pid_table.o
	$(CC) $(CFLAGS) -o $@ main.o pid_table.o

tilera: main.o parcer.o pid_table.o
	$(TILECC) $(CFLAGS) -o $@ main.o pid_table.o
