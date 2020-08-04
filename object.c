#include "memory.h"
#include "object.h"

object *create_new_object(objtype type)
{
	object *obj = ALLOCATE(object, 1);
	obj->type = type;
	return obj;
}

objprim inline *init_primitive_number(object *obj, double value)
{
	objprim *new_prim = (objprim*)obj;
	newprim->type = OBJ_PRIMITIVE;
	newprim->primtype = VAL_NUMBER;
	newprim->val_number = value;
	return new_prim;
}

objprim inline *init_primitive_bool(object *obj, int value)
{
	objprim *new_prim = (objprim*)obj;
	newprim->type = OBJ_PRIMITIVE;
	newprim->primtype = VAL_BOOL;
	newprim->val_int = value;
	return new_prim;
}

typedef enum
{
	OBJ_PRIMITIVE,
	OBJ_CLASS,
	OBJ_INSTANCE,
} objtype;

typedef enum
{
	VAL_NUMBER,
	VAL_STRING,
	VAL_BOOL,
	VAL_NULL,
} valtype;

typedef struct object_t
{
	objtype type;
} object;

typedef struct objprim_t
{
	object obj;
	valtype primtype;
	union
	{
		int val_int;
		double val_number;
		char *val_string;
	};
} objprim;

object *create_new_object(objtype type);
objprim *init_primtive_null(object *obj, int value);
objprim *init_primitie_string(object *obj, char *value);

