CC=gcc
CFLAGS=-Wall -Wextra -std=c99
LDFLAGS=-lm

all: program

program: main.o serwer.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

main.o: main.c serwer.h
	$(CC) $(CFLAGS) -c $< -o $@

serwer.o: serwer.c serwer.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o program
