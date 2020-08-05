#include "compiler.h"
#include "memory.h"
#include "vm.h"

void interpret(char *source)
{}

void interpret_line(VM *vm, char *source)
{
	instruct *instructs = compile(&vm->analyzer, source);
    execute(vm, instructs);
	FREE_OBJECT(instruct, instructs);
}
