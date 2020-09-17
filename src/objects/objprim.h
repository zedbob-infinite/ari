#ifndef ari_objprim_h
#define ari_objprim_h

#define PRIM_AS_BOOL(obj)		        (obj->val_int)
#define PRIM_AS_DOUBLE(obj)		        (obj->val_double)
#define PRIM_AS_NULL(obj)		        (obj->val_int)
#define PRIM_AS_STRING(obj)		        (obj->val_string)
#define PRIM_AS_RAWSTRING(obj)          (obj->val_string->_string_)

#define PRIM_DOUBLE_VAL(obj, value)     (obj->val_double = value)
#define PRIM_BOOL_VAL(obj, value)		(obj->val_int = value)
#define PRIM_NULL_VAL(obj)			    (obj->val_int = 0)

#define PRIMSTRING_AS_RAWSTRING(obj)    (obj->_string_)

#include "object.h"
#include "token.h"

typedef enum
{
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
int hashkey(char *key, int length);
primstring *create_primstring(char *_string_);
primstring *init_primstring(int length, uint32_t hash, char *takenstring);
void convert_prim(objprim *prim, primtype type);
void free_primstring(primstring *del);
bool check_zero_div(object *a, object *b);
object *binary_comp(objprim *a, objprim *b, tokentype optype);

#endif
