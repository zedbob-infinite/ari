#ifndef ari_builtin_h
#define ari_builtin_h

#include "object.h"

typedef object *(*builtin)(int argcount, object **args);

typedef struct
{
    object header;
    builtin func;
} objbuiltin;


object *builtin_println(int argcount, object **args);
object *builtin_input(int argcount, object **args);
object *builtin_type(int argcount, object **args);

#endif
