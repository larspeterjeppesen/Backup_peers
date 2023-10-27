CC=gcc
CFLAGS=-g -Wall -Wextra -pedantic -std=gnu11
LD_FLAGS= -lpthread -lm


client: client.c client.h io_assist.o sha256.o
	$(CC) $(CFLAGS) $< *.o -o $@  $(LD_FLAGS)

io_assist.o: io_assist.c io_assist.h
	$(CC) $(CFLAGS) -c $< -o $@

sha256.o: sha256.c sha256.h
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf *.o client 
