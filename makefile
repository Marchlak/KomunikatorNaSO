CC=gcc
CFLAGS=-Wall -g

all: program

program: main.o klient.o serwer.o wiadomosci.o
	$(CC) $(CFLAGS) main.o klient.o serwer.o wiadomosci.o -o program

main.o: main.c klient.h serwer.h wiadomosci.h
	$(CC) $(CFLAGS) -c main.c

klient.o: klient.c klient.h wiadomosci.h
	$(CC) $(CFLAGS) -c klient.c

serwer.o: serwer.c klient.h serwer.h wiadomosci.h
	$(CC) $(CFLAGS) -c serwer.c

wiadomosci.o: wiadomosci.c wiadomosci.h
	$(CC) $(CFLAGS) -c wiadomosci.c

clean:
	rm -f *.o program

