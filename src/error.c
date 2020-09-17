#include <stdio.h>

#include "error.h"

intrpstate runtime_error(VM *vm, objstack *stack, size_t line, 
        const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    fprintf(stderr, "[line %ld] in script\n", line);

    reset_objstack(stack);
    reset_vm(vm);
    while (vm->framestackpos > 0)
        vm_pop_frame(vm);
    return INTERPRET_RUNTIME_ERROR;
}

intrpstate runtime_error_loadname(VM *vm, char *name, uint64_t current)
{
    char msg[100];
    sprintf(msg, "Name %s not found...", name);
    return runtime_error(vm, &vm->evalstack, current, msg);
}

intrpstate runtime_error_unsupported_operation(VM *vm,
        uint64_t current, char optype)
{
    char msg[50];
    sprintf(msg, "Error: unsupported operand type for %c", optype);
    return runtime_error(vm, &vm->evalstack, current, msg);
}

intrpstate runtime_error_zero_div(VM *vm, uint64_t current)
{
    char msg[50];
    sprintf(msg, "Error: Divison by Zero");
    return runtime_error(vm, &vm->evalstack, current, msg);
}
