#include <stdio.h>

#include "compiler.h"
#include "interpret.h"
#include "instruct.h"
#include "memory.h"

void interpret(AriFile *file)
{
    const char *source = file->source;
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
