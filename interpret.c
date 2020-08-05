#include <stdio.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

void interpret(char *source)
{}

void interpret_line(VM *vm, char *source)
{
	instruct *instructs = compile(&vm->analyzer, source);
    if (!instructs)
        return;
    execute(vm, instructs);
    printf("entering reset_instruct()...\n");
    reset_instruct(instructs);
    printf("exiting reset_instruct()...\n");
	FREE(instruct, instructs);
}
