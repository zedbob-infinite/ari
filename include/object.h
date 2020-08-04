#ifndef ari_object_h
#define ari_object_h

#define PRIM_AS_BOOL(obj)		(obj->val_int)
#define PRIM_AS_NUMBER(obj)		(obj->val_number)
#define PRIM_AS_NULL(obj)		(obj->val_int)
#define PRIM_AS_STRING(obj)		(obj->val_string)

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
objprim *init_primitive_number(object *obj, double value);
objprim *init_primitive_bool(object *obj, int value);
objprim *init_primtive_null(object *obj, int value);
objprim *init_primitie_string(object *obj, char *value);

#endif
