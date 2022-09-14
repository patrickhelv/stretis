SHELL = /bin/sh

CC = gcc


start: 
	$(CC) stetris.c -o stetris.o

run:
	./stetris.o

clean: 
	-rm -f *.o