CC=gcc
CFLAGS=-g -Wall -Wextra -pedantic -std=gnu11
LD_FLAGS= -lpthread -lm


client: client.c client.h compsys_helpers.o sha256.o
	$(CC) $(CFLAGS) $< *.o -o $@  $(LD_FLAGS)

compsys_helpers.o: compsys_helpers.c compsys_helpers.h
	$(CC) $(CFLAGS) -c $< -o $@

sha256.o: sha256.c sha256.h
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf *.o client 
