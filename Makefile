INC=-I./include -I./objects -I./parser
CC = gcc
CFLAGS = -g -Wall -Wshadow
LDFLAGS = -g

vmmake: main.c object.o objclass.o objstack.o objhash.o objcode.o frame.o module.o interpret.o instruct.o repl.o vm.o compiler.o tokenizer.o parser.o memory.o
	$(CC) $(LDFLAGS) $(INC) instruct.o objclass.o frame.o objcode.o interpret.o module.o tokenizer.o objhash.o objstack.o compiler.o repl.o object.o vm.o parser.o memory.o main.c -o ari

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

parser.o: parser/parser.c
	$(CC) $(CFLAGS) $(INC) -c parser/parser.c

tokenizer.o: parser/tokenizer.c
	$(CC) $(CFLAGS) $(INC) -c parser/tokenizer.c

compiler.o: compiler.c
	$(CC) $(CFLAGS) $(INC) -c compiler.c

object.o: objects/object.c
	$(CC) $(CFLAGS) $(INC) -c objects/object.c

objcode.o: objects/objcode.c
	$(CC) $(CFLAGS) $(INC) -c objects/objcode.c

objclass.o: objects/objclass.c
	$(CC) $(CFLAGS) $(INC) -c objects/objclass.c

objstack.o: objstack.c
	$(CC) $(CFLAGS) $(INC) -c objstack.c

vm.o: vm.c
	$(CC) $(CFLAGS) $(INC) -c vm.c

repl.o: repl.c
	$(CC) $(CFLAGS) $(INC) repl.c -c -DVERSION='"9.3.0"' -DOS='"linux"'

clean:
	rm *.o
