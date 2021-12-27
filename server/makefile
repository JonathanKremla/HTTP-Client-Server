CC = gcc

FLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g
OBJ = server.o 

.PHONY: all clean
all: server

server: server.o
	$(CC) $(FLAGS) -o $@ $^ 

%.o: %.c
	$(CC) $(FLAGS) -c -o $@ $<

clean:
	rm -rf *.o server
