#ifndef ari_objprim_h
#define ari_objprim_h

#define PRIM_AS_BOOL(obj)		(obj->val_int)
#define PRIM_AS_INT(obj)		(obj->val_int)
#define PRIM_AS_DOUBLE(obj)		(obj->val_double)
#define PRIM_AS_NULL(obj)		(obj->val_int)
#define PRIM_AS_STRING(obj)		(obj->val_string)
#define PRIM_AS_RAWSTRING(obj)  (obj->val_string->_string_)

#define PRIM_INT_VAL(obj, value)		(obj->val_int = value)
#define PRIM_DOUBLE_VAL(obj, value)     (obj->val_double = value)
#define PRIM_BOOL_VAL(obj, value)		(obj->val_int = value)
#define PRIM_NULL_VAL(obj)			    (obj->val_int = 0)

#include "object.h"
#include "token.h"

typedef enum
{
	PRIM_INT,
	PRIM_DOUBLE,
	PRIM_STRING,
	PRIM_BOOL,
	PRIM_NULL,
} primtype;

typedef struct primstring_t
{
    int length;
    char *_string_;
    uint32_t hash;
} primstring;

typedef struct objprim_t
{
	object header;
	primtype ptype;
	union
	{
		int val_int;
		double val_double;
		primstring *val_string;
	};
} objprim;

objprim *create_new_primitive(primtype ptype);
void construct_primstring(objprim *primobj, char *string_);
void construct_primstring_from_token(objprim *primobj, token *tok);

#endif
