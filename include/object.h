#ifndef ari_object_h
#define ari_object_h

#define PRIM_AS_BOOL(obj)		(obj->val_int)
#define PRIM_AS_INT(obj)		(obj->val_int)
#define PRIM_AS_DOUBLE(obj)		(obj->val_double)
#define PRIM_AS_NULL(obj)		(obj->val_int)
#define PRIM_AS_STRING(obj)		(obj->val_string)

#define INT_VAL(obj, value)					(obj->val_int = value)
#define DOUBLE_VAL(obj, value)              (obj->val_double = value)
#define BOOL_VAL(obj, value)				(obj->val_int = value)
#define NULL_VAL(obj)						(obj->val_int = 0)

#define ALLOCATE_PRIM_STRING(obj, length)	(obj->val_string = ALLOCATE(char, length + 1))
#define COPY_PRIM_STRING(obj, string, length)		(strncpy(obj->val_string, string, length))

typedef enum
{
	OBJ_PRIMITIVE,
	OBJ_CLASS,
	OBJ_INSTANCE,
} objtype;

typedef enum
{
	VAL_INT,
	VAL_DOUBLE,
	VAL_STRING,
	VAL_BOOL,
	VAL_NULL,
} valtype;

typedef struct object_t
{
	objtype type;
    struct object_t* next;
} object;

typedef struct objprim_t
{
	object obj;
	valtype primtype;
	union
	{
		int val_int;
		double val_double;
		char *val_string;
	};
} objprim;

object *create_new_object(objtype type);
objprim *create_new_primitive(valtype primtype);
void print_object(object *obj);

#endif
