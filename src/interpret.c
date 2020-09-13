#include <stdio.h>

#include "compiler.h"
#include "instruct.h"
#include "memory.h"
#include "vm.h"

void interpret(char *source)
{
    VM *vm = init_vm();
    init_instruct(&vm->global.instructs);
	vm->global.instructs = compile(&vm->analyzer, source);
    if (!vm->global.instructs.count) {
        reset_instruct(&vm->global.instructs);
        return;
    }

    execute(vm, &vm->global.instructs);
    reset_instruct(&vm->global.instructs);
    free_vm(vm);
}

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
