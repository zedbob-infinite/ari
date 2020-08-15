#include "frame.h"
#include "instruct.h"
#include "memory.h"
#include "objcode.h"

objcode *init_objcode(int argcount, objprim **arguments)
{
	objcode *codeobj = ALLOCATE(objcode, 1);
	codeobj->obj.type = OBJ_CODE;
	codeobj->argcount = argcount;
	codeobj->arguments = arguments;
	init_frame(&codeobj->localframe);
	init_instruct(&codeobj->instructs);
	return codeobj;
}
