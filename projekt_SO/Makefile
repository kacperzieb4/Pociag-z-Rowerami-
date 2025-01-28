CC = gcc
CFLAGS = -Wall -O2

all: zawiadowca kierownik pasazer main

funkcje.o: funkcje.c funkcje.h
	$(CC) $(CFLAGS) -c funkcje.c

zawiadowca: zawiadowca.c funkcje.o
	$(CC) $(CFLAGS) zawiadowca.c funkcje.o -o zawiadowca

kierownik: kierownik.c funkcje.o
	$(CC) $(CFLAGS) kierownik.c funkcje.o -o kierownik

pasazer: pasazer.c funkcje.o
	$(CC) $(CFLAGS) pasazer.c funkcje.o -o pasazer

main: main.c funkcje.o
	$(CC) $(CFLAGS) main.c funkcje.o -o main

clean:
	rm -f *.o zawiadowca kierownik pasazer main
