#include <stdio.h>

#include "memory.h"
#include "object.h"

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
			init_null(obj);
			break;
		case PRIM_STRING:
			init_string(obj);
			break;
	}
	return obj;
}

void init_object(object *obj, objtype type, object *next)
{
    obj->type = type;
    obj->next = next;
}

void print_object(object *obj)
{
    if (!obj) {
        printf("No object found.\n");
        return;
    }
	switch (obj->type) {
		case OBJ_PRIMITIVE:
		{
			objprim *prim = (objprim*)obj;
			switch (prim->ptype) {
				case PRIM_INT:
					printf("%d\n", PRIM_AS_INT(prim));
					break;
				case PRIM_DOUBLE:
					printf("%f\n", PRIM_AS_DOUBLE(prim));
					break;
				case PRIM_STRING:
					printf("%s\n", PRIM_AS_STRING(prim));
					break;
				case PRIM_BOOL:
					printf("%s\n", PRIM_AS_BOOL(prim) ? "true" : "false");
					break;
				case PRIM_NULL:
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
