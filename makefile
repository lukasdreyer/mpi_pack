CC=mpicc
CFLAGS=-g3 -O0 -I.

all: main.o offset.o element_list.o
	$(CC) $(CFLAGS) -o mpi_pack_test main.o offset.o element_list.o 
