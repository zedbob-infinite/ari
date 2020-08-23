#include "memory.h"
#include "objprim.h"

static void inline init_int(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_INT;
	obj->val_int = 0;
}

static void inline init_double(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_DOUBLE;
	obj->val_double = 0;
}

static void inline init_bool(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_BOOL;
	obj->val_int = 0;
}

static void inline init_null(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_NULL;
	obj->val_int = 0;
}

static void inline init_string(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_STRING;
	obj->val_string = NULL;
}

objprim *create_new_primitive(primtype ptype)
{
	objprim *obj = ALLOCATE(objprim, 1);
	
	switch (ptype) {
		case PRIM_INT:
			init_int(obj);
			break;
		case PRIM_DOUBLE:
			init_double(obj);
			break;
		case PRIM_BOOL:
			init_bool(obj);
			break;
		case PRIM_NULL:
