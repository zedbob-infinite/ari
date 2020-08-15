#ifndef ari_objcode_h
#define ari_objcode_h

#include "frame.h"
#include "instruct.h"
#include "object.h"

typedef struct objcode_t
{
	object obj;
	int argcount;
	objprim **arguments;
	frame localframe;
	instruct instructs;
} objcode;

objcode *init_objcode(int argcount, objprim **arguments);

#endif
