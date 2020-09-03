#ifndef ari_vm_h
#define ari_vm_h

#include "instruct.h"
#include "frame.h"
#include "module.h"
#include "object.h"
#include "objhash.h"
#include "objstack.h"
#include "parser.h"
#include "tokenizer.h"

typedef enum
{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
} intrpstate;

typedef struct VM_t
{
    parser analyzer;
	objstack evalstack;
    module global;
    frame *top;
    object *objs;
    int num_objects;
    int callstackpos;
    int framestackpos; 
} VM;

void interpret(char *source);
void interpret_line(VM *vm, char *source);
/*void run_vm_from_file(FILE* file);*/
VM *init_vm(void);
void free_vm(VM *vm);
void reset_vm(VM *vm);
intrpstate execute(VM *vm, instruct *instructs);
void print_value(value val, valtype type);

#endif
