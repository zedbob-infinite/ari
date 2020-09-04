#include <limits.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "objprim.h"
#include "token.h"

static int hashkey(char *key, int length)
{
    uint32_t hashval = 0;
    int i = 0;

    // Convert string to integer
    while (hashval < ULONG_MAX && i < length) {
        hashval <<= 8;
        hashval += key[i];
        i++;
    }
    return hashval;
}

static void inline init_int(objprim *obj)
{
	obj->header.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_INT;
	obj->val_int = 0;
}

static void inline init_double(objprim *obj)
{
	obj->header.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_DOUBLE;
	obj->val_double = 0;
}

static void inline init_bool(objprim *obj)
{
	obj->header.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_BOOL;
	obj->val_int = 0;
}

static void inline init_null(objprim *obj)
{
	obj->header.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_NULL;
	obj->val_int = 0;
}

static void inline init_string(objprim *obj)
{
	obj->header.type = OBJ_PRIMITIVE;
	obj->ptype = PRIM_STRING;
	obj->val_string = NULL;
}

static primstring *init_primstring(int length, uint32_t hash, 
        char *takenstring)
{
    primstring *newstring = ALLOCATE(primstring, 1);
    newstring->length = length;
    newstring->hash = hash;
    newstring->_string_ = takenstring;
    return newstring;
}

void construct_primstring(objprim *primobj, char *_string_)
{
    int length = strlen(_string_) - 2;
    uint32_t hash = hashkey(_string_, length);
    char *takenstring = ALLOCATE(char, length);
    memcpy(takenstring, _string_ + 1, length);

    PRIM_AS_STRING(primobj) = init_primstring(length, hash, takenstring);
}

void construct_primstring_from_token(objprim *primobj, token *tok)
{
    int length = tok->length;
    char *takenstring = ALLOCATE(char, length + 1);
    memcpy(takenstring, tok->start, tok->length);
    takenstring[tok->length] = '\0';
    
    uint32_t hash = hashkey(takenstring, length);
    PRIM_AS_STRING(primobj) = init_primstring(length,  hash, takenstring); 
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
