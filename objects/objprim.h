#ifndef ari_objprim_h
#define ari_objprim_h

#define PRIM_AS_BOOL(obj)		(obj->val_int)
#define PRIM_AS_INT(obj)		(obj->val_int)
#define PRIM_AS_DOUBLE(obj)		(obj->val_double)
#define PRIM_AS_NULL(obj)		(obj->val_int)
#define PRIM_AS_STRING(obj)		(obj->val_string)

#define PRIM_INT_VAL(obj, value)		(obj->val_int = value)
#define PRIM_DOUBLE_VAL(obj, value)     (obj->val_double = value)
#define PRIM_BOOL_VAL(obj, value)		(obj->val_int = value)
#define PRIM_NULL_VAL(obj)			    (obj->val_int = 0)

#define ALLOCATE_PRIM_STRING(obj, length)	(obj->val_string = ALLOCATE(char, length + 1))
#define COPY_PRIM_STRING(obj, string, length)		(strncpy(obj->val_string, string, length))

typedef struct object;

typedef enum
{
	PRIM_INT,
	PRIM_DOUBLE,
	PRIM_STRING,
	PRIM_BOOL,
	PRIM_NULL,
} primtype;

typedef struct objprim_t
{
	object header;
	primtype ptype;
	union
	{
		int val_int;
		double val_double;
		char *val_string;
	};
} objprim;

objprim *create_new_primitive(primtype ptype);

#endif
