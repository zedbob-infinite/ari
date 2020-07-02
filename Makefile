INC=-I./include
CC = gcc
CFLAGS = -g -Wall -Wshadow

vmmake: main.c repl.c compile.c
	$(CC) $(CFLAGS) $(INC) compile.c repl.c main.c -o ./build/ari

clean:
	rm *.o
