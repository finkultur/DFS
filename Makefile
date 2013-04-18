CC=gcc
TILECC=/opt/tilepro/bin/tile-cc
CCFLAGS= -Wall #-std=c99
LNFLAGS= -ltmc

tilera: main.o cmd_list.o 
	$(TILECC) $(CCFLAGS) $(LNFLAGS) -o main main.o cmd_list.o 

main.o: main.c
	$(TILECC) $(CCFLAGS) -c main.c main.o

cmd_list.o: cmd_list.c cmd_list.h
	$(TILECC) $(CCFLAGS) -c cmd_list.c cmd_list.o
