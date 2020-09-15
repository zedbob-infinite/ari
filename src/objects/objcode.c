#include "frame.h"
#include "instruct.h"
#include "memory.h"
#include "object.h"
#include "objcode.h"

objcode *init_objcode(int argcount, objprim **arguments)
{
	objcode *codeobj = ALLOCATE(objcode, 1);
	codeobj->argcount = argcount;
	codeobj->arguments = arguments;
    codeobj->depth = 0;
    init_object(codeobj, OBJ_CODE);
	init_frame(&codeobj->localframe);
	init_instruct(&codeobj->instructs);
	return codeobj;
}
