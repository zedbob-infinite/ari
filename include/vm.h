#ifndef ari_vm_h
#define ari_vm_h

#include "instruct.h"
#include "object.h"
#include "objhash.h"
#include "objstack.h"
#include "parser.h"
#include "tokenizer.h"

typedef enum
{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
} interpret_states;

typedef struct VM_t
{
    parser analyzer;
	objstack evalstack;
    objhash globals;
	instruct *instructs;
    object *objs;
    int num_objects;
} VM;

void interpret(char *source);
void interpret_line(VM *vm, char *source);
/*void run_vm_from_file(FILE* file);*/
VM *init_vm(void);
void free_vm(VM *vm);
int execute(VM *vm, instruct *instructs);

#endif
