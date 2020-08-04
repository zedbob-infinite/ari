#include "compiler.h"
#include "vm.h"


void interpret_line(VM *vm, char *source)
{
    compile(vm, source);
    /*instruct *instructs;
    instructs = compile(vm, source);
    execute(vm, instructs);
    free(instructs);*/
}
