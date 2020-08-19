#ifndef ari_objcode_h
#define ari_objcode_h

#include <stddef.h>

#include "frame.h"
#include "instruct.h"
#include "object.h"

typedef struct objcode_t
{
	object obj;
	size_t argcount;
	objprim **arguments;
	frame localframe;
	instruct instructs;
    size_t depth;
} objcode;

objcode *init_objcode(int argcount, objprim **arguments);

#endif
