CC=gcc
CFLAGS=-Wall -g

all: program

program: main.o klient.o serwer.o
	$(CC) $(CFLAGS) main.o klient.o serwer.o -o program

main.o: main.c klient.h serwer.h
	$(CC) $(CFLAGS) -c main.c

klient.o: klient.c klient.h
	$(CC) $(CFLAGS) -c klient.c

serwer.o: serwer.c klient.h serwer.h
	$(CC) $(CFLAGS) -c serwer.c

clean:
	rm -f *.o program
