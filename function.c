#include "function.h"

void init_function(objfunction *function)
{
typedef struct object_t
{
	objtype type;
    struct object_t* next;
} object;

typedef struct objprim_t
{
	object obj;
	primtype ptype;
	union
	{
		int val_int;
		double val_double;
		char *val_string;
	};
} objprim;
