#include <stdio.h>

#include "memory.h"
#include "object.h"

static void inline init_int(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->primtype = VAL_INT;
	obj->val_int = 0;
}

static void inline init_double(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->primtype = VAL_DOUBLE;
	obj->val_double = 0;
}

static void inline init_bool(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->primtype = VAL_BOOL;
	obj->val_int = 0;
}

static void inline init_null(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->primtype = VAL_NULL;
	obj->val_int = 0;
}

static void inline init_string(objprim *obj)
{
	obj->obj.type = OBJ_PRIMITIVE;
	obj->primtype = VAL_STRING;
	obj->val_string = NULL;
}

objprim *create_new_primitive(valtype primtype)
{
	objprim *obj = ALLOCATE(objprim, 1);
	
	switch (primtype) {
		case VAL_INT:
			init_int(obj);
			break;
		case VAL_DOUBLE:
			init_double(obj);
			break;
		case VAL_BOOL:
			init_bool(obj);
			break;
		case VAL_NULL:
			init_null(obj);
			break;
		case VAL_STRING:
			init_string(obj);
			break;
	}
	return obj;
}

/*object *create_new_object(objtype type)
{
	switch (type) {
		case OBJ_PRIMITIVE:
			object *obj = NULL;
		default:
			return NULL;
	}
	return (object*)obj;
}*/

void print_object(object *obj)
{
	switch (obj->type) {
		case OBJ_PRIMITIVE:
		{
			objprim *prim = (objprim*)obj;
			switch (prim->primtype) {
				case VAL_INT:
					printf("%d\n", PRIM_AS_INT(prim));
					break;
				case VAL_DOUBLE:
					printf("%f\n", PRIM_AS_DOUBLE(prim));
					break;
				case VAL_STRING:
					printf("%s\n", PRIM_AS_STRING(prim));
					break;
				case VAL_BOOL:
					printf("%s\n", PRIM_AS_BOOL(prim) ? "true" : "false");
					break;
				case VAL_NULL:
					printf("null\n");
					break;
				default:
					break;
			}
			break;
		default:
			break;
		}
	}
}
