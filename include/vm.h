#ifndef ari_vm_h
#define ari_vm_h

#include "instruct.h"
#include "object.h"
#include "objstack.h"
#include "parser.h"
#include "tokenizer.h"

typedef struct VM_t
{
    parser analyzer;
	objstack evalstack;
	instruct *instructs;
    object *objs;
} VM;

void interpret(char *source);
void interpret_line(VM *vm, char *source);
/*void run_vm_from_file(FILE* file);*/
VM *init_vm(void);
void free_vm(VM *vm);
void execute(VM *vm, instruct *instructs);

#endif
