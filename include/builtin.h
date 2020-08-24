#ifndef ari_builtin_h
#define ari_builtin_h

#include "object.h"

typedef object *(*builtin)(int argcount, object **args);

typedef struct
{
    object header;
    builtin func;
} objbuiltin;

#endif
