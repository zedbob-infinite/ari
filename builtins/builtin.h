#ifndef ari_builtin_h
#define ari_builtin_h

#include "object.h"
#include "vm.h"

typedef object *(*builtin)(VM *vm, int argcount, object **args);

typedef struct
{
    object header;
    builtin func;
} objbuiltin;


object *call_builtin(VM *vm, object *obj, int argcount, 
        object **arguments);
object *load_builtin(VM *vm, char *name, builtin function);

object *builtin_println(VM *vm, int argcount, object **args);
object *builtin_input(VM *vm, int argcount, object **args);
object *builtin_type(VM *Vm, int argcount, object **args);

#endif
