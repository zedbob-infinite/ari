INC=-I./include
CC = gcc
CFLAGS = -g -Wall -Wshadow

vmmake: main.c object.o objstack.o objhash.o objcode.o frame.o module.o interpret.o instruct.o repl.o vm.o compiler.o tokenizer.o parser.o memory.o
	$(CC) $(CFLAGS) $(INC) instruct.o frame.o objcode.o interpret.o module.o tokenizer.o objhash.o objstack.o compiler.o repl.o object.o vm.o parser.o memory.o main.c -o ari

objcode.o: objcode.c
	$(CC) $(CFLAGS) $(INC) -c objcode.c

frame.o: frame.c
	$(CC) $(CFLAGS) $(INC) -c frame.c

module.o: module.c
	$(CC) $(CFLAGS) $(INC) -c module.c

objhash.o: objhash.c
	$(CC) $(CFLAGS) $(INC) -c objhash.c

instruct.o: instruct.c
	$(CC) $(CFLAGS) $(INC) -c instruct.c

memory.o: memory.c
	$(CC) $(CFLAGS) $(INC) -c memory.c

interpret.o: interpret.c
	$(CC) $(CFLAGS) $(INC) -c interpret.c

parser.o: parser.c
	$(CC) $(CFLAGS) $(INC) -c parser.c

compiler.o: compiler.c
	$(CC) $(CFLAGS) $(INC) -c compiler.c

tokenizer.o: tokenizer.c
	$(CC) $(CFLAGS) $(INC) -c tokenizer.c

object.o: object.c
	$(CC) $(CFLAGS) $(INC) -c object.c

objstack.o: objstack.c
	$(CC) $(CFLAGS) $(INC) -c objstack.c

vm.o: vm.c
	$(CC) $(CFLAGS) $(INC) -c vm.c

repl.o: repl.c
	$(CC) $(CFLAGS) $(INC) repl.c -c -DVERSION='"9.3.0"' -DOS='"linux"'

clean:
	rm *.o
