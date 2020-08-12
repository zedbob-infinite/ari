#include <stdio.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

void interpret(char *source)
{}

void interpret_line(VM *vm, char *source)
{
	vm->global.instructs = compile(&vm->analyzer, source);
    if (!vm->global.instructs.count) {
        reset_instruct(&vm->global.instructs);
        return;
    }

    execute(vm, &vm->global.instructs);
    reset_instruct(&vm->global.instructs);
}
