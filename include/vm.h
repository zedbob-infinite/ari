#ifndef ari_vm_h
#define ari_vm_h

#include "parser.h"
#include "tokenizer.h"
#include "valstack.h"

typedef struct VM_t
{
    parser analyzer;
    valstack evalstack;
} VM;

void interpret(char *source);
void interpret_line(VM *vm, char *source);
/*void run_vm_from_file(FILE* file);*/
VM *init_vm(void);

#endif
