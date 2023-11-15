CC=gcc
CFLAGS=-g -Wall -Wextra -pedantic -std=gnu11
LD_FLAGS= -lpthread -lm


all: confidant io_assist.o sha256.o transfer.o job_queue.o

confidant: confidant.c confidant.h transfer.o job_queue.o io_assist.o sha256.o
	$(CC) $(CFLAGS) $< *.o -o $@  $(LD_FLAGS)

transfer.o: transfer.c transfer.h
	$(CC) $(CFLAGS) -c $< -o $@

job_queue.o: job_queue.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LD_FLAGS)

io_assist.o: io_assist.c io_assist.h
	$(CC) $(CFLAGS) -c $< -o $@

sha256.o: sha256.c sha256.h
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf *.o confidant
