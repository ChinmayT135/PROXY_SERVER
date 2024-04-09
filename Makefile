CC = gcc
CFLAGS = -c -Wall -lpthread
OBJS = server.o parse.o lru_cache.o

util_server : server.o parse.o lru_cache.o
	$(CC) $(OBJS) -o util_server

server.o : server.c headers.h
	$(CC) $(CFLAGS) -lpthread server.c headers.h

parse.o : parse.c
	$(CC) $(CFLAGS) parse.c

lru_cache.o : lru_cache.c
	$(CC) $(CFLAGS) lru_cache.c

clean: 
	rm -rf *.o util_server
