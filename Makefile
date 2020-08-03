INC=-I./include
CC = gcc
CFLAGS = -g -Wall -Wshadow

vmmake: main.c value.o valstack.o interpret.o repl.o vm.o compiler.o tokenizer.o parser.o
	$(CC) $(CFLAGS) $(INC) interpret.o tokenizer.o valstack.o compiler.o repl.o value.o vm.o parser.o main.c -o ari

interpret.o: interpret.c
	$(CC) $(CFLAGS) $(INC) -c interpret.c

parser.o: parser.c
	$(CC) $(CFLAGS) $(INC) -c parser.c

compiler.o: compiler.c
	$(CC) $(CFLAGS) $(INC) -c compiler.c

tokenizer.o: tokenizer.c
	$(CC) $(CFLAGS) $(INC) -c tokenizer.c

value.o: value.c
	$(CC) $(CFLAGS) $(INC) -c value.c

valstack.o: valstack.c
	$(CC) $(CFLAGS) $(INC) -c valstack.c

vm.o: vm.c
	$(CC) $(CFLAGS) $(INC) -c vm.c

repl.o: repl.c
	$(CC) $(CFLAGS) $(INC) repl.c -c -DVERSION='"9.3.0"' -DOS='"linux"'

clean:
	rm *.o
