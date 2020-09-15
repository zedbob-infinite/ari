#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "objprim.h"
#include "token.h"

object *prim_binary_add(object *this, object *other);
object *prim_binary_sub(object *this, object *other);
object *prim_binary_mul(object *this, object *other);
object *prim_binary_div(object *this, object *other);

int hashkey(char *key, int length)
{
    uint32_t hashval = 0;
    int i = 0;

    // Convert string to integer
    while (hashval < UINT32_MAX && i < length) {
        hashval <<= 8;
        hashval += key[i];
        i++;
    }
    return hashval;
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

primstring *init_primstring(int length, uint32_t hash, char *takenstring)
{
    primstring *newstring = ALLOCATE(primstring, 1);
    newstring->length = length;
    newstring->hash = hash;
    newstring->_string_ = takenstring;
    return newstring;
}

void free_primstring(primstring *del)
{
    FREE(char, del->_string_);
    FREE(primstring, del);
}

primstring *create_primstring(char *_string_)
{
    int length = strlen(_string_);
    uint32_t hash = hashkey(_string_, length);
    char *takenstring = ALLOCATE(char, length + 1);
    memcpy(takenstring, _string_, length);
    takenstring[length] = '\0';

    return init_primstring(length, hash, takenstring);
}

objprim *create_new_primitive(primtype ptype)
{
	objprim *obj = ALLOCATE(objprim, 1);
    init_object(obj, OBJ_PRIMITIVE);

	switch (ptype) {
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
    obj->header.__add__ = prim_binary_add;
    obj->header.__sub__ = prim_binary_sub;
    obj->header.__mul__ = prim_binary_mul;
    obj->header.__div__ = prim_binary_div;
	return obj;
}

void convert_prim(objprim *prim, primtype type)
{
    primtype current = prim->ptype;
    switch (current) {
        case PRIM_DOUBLE:
        {
            if (type == PRIM_DOUBLE)
                return;
            else if (type == PRIM_STRING) {
                double newdouble = PRIM_AS_DOUBLE(prim);
                sprintf(PRIM_AS_RAWSTRING(prim), "%lf", newdouble);
                prim->ptype = PRIM_DOUBLE;
            }
            else if (type == PRIM_BOOL) {
                PRIM_AS_BOOL(prim) = (PRIM_AS_DOUBLE(prim) >= 1 ? true : false);
                prim->ptype = PRIM_BOOL;
            }
            else if (type == PRIM_NULL) {
                PRIM_AS_NULL(prim) = 0;
                prim->ptype = PRIM_NULL;
            }
        }
        case PRIM_STRING:
        {
            if (type == PRIM_STRING)
                return;
            else if (type == PRIM_DOUBLE) {
                double newdouble = atof(PRIM_AS_RAWSTRING(prim));
                PRIM_AS_DOUBLE(prim) = newdouble;
                prim->ptype = PRIM_DOUBLE;
            }
            else if (type == PRIM_BOOL)
                return;
            else if (type == PRIM_NULL) {
                PRIM_AS_NULL(prim) = 0;
                prim->ptype = PRIM_NULL;
            }
        }
        case PRIM_BOOL:
        {
            if (type == PRIM_BOOL)
                return;
            else if (type == PRIM_DOUBLE) {
                double newdouble = (double)PRIM_AS_BOOL(prim);
                PRIM_AS_DOUBLE(prim) = newdouble;
                prim->ptype = PRIM_DOUBLE;
            }
            else if (type == PRIM_STRING)
                return;
            else if (type == PRIM_NULL) {
                PRIM_AS_NULL(prim) = 0;
                prim->ptype = PRIM_NULL;
            }

        }
        default:
            return;
    }
}

static bool match(objprim *a, objprim *b, primtype type)
{
    return ((a->ptype == type) || (b->ptype == type));
}

static objprim *concatenate(objprim *a, objprim *b)
{
    primstring *string_a = PRIM_AS_STRING(a);
    primstring *string_b = PRIM_AS_STRING(b);

    int length = string_a->length + string_b->length;
    char *newstring = ALLOCATE(char, length + 1);
    memcpy(newstring, string_a->_string_, string_a->length);
    memcpy(newstring + string_a->length, string_b->_string_, string_b->length);
    newstring[length] = '\0';

    uint32_t hash = hashkey(newstring, length);

    objprim *newprim = create_new_primitive(PRIM_STRING);
    PRIM_AS_STRING(newprim) = init_primstring(length, hash, newstring);
    return newprim;
}

static objprim *repeat(objprim *a, int times)
{
    primstring *string_a = PRIM_AS_STRING(a);

    int length = string_a->length * times;
    char *newstring = ALLOCATE(char, length + 1);
    memcpy(newstring, string_a->_string_, string_a->length);
    for (int i = 0; i < times; i++)
        memcpy(newstring + (string_a->length * (i +1)), string_a->_string_,
                string_a->length);
    newstring[length] = '\0';

    uint32_t hash = hashkey(newstring, length);

    objprim *newprim = create_new_primitive(PRIM_STRING);
    PRIM_AS_STRING(newprim) = init_primstring(length, hash, newstring);
    return newprim;
}

object *prim_binary_add(object *this_, object *other)
{
    if (this_->type != OBJ_PRIMITIVE || other->type != OBJ_PRIMITIVE)
        return NULL;

    objprim *a = (objprim*)this_;
    objprim *b = (objprim*)other;
    objprim *c = NULL;

    if (match(a, b, PRIM_DOUBLE)) {
        if ((a->ptype == PRIM_DOUBLE && b->ptype == PRIM_DOUBLE)) {
            c = create_new_primitive(PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) + PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_BOOL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_BOOL)) {
            c = create_new_primitive(PRIM_DOUBLE);
            convert_prim(a, PRIM_DOUBLE);
            convert_prim(b, PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) + PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_STRING && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_NULL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_NULL)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_STRING)) {
        if ((a->ptype == PRIM_STRING && b->ptype == PRIM_STRING)) {
            c = concatenate(a, b);
        }
        else if ((a->ptype == PRIM_STRING && b->ptype == PRIM_BOOL) ||
                 (a->ptype == PRIM_BOOL && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_STRING && b->ptype == PRIM_NULL) ||
                 (a->ptype == PRIM_NULL && b->ptype == PRIM_STRING)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_BOOL)) {
        if (a->ptype == PRIM_BOOL && b->ptype == PRIM_BOOL) {
            c = create_new_primitive(PRIM_BOOL);
            PRIM_AS_BOOL(c) = PRIM_AS_BOOL(a) + PRIM_AS_BOOL(b);
        }
        else if ((a->ptype == PRIM_NULL && b->ptype == PRIM_BOOL) ||
                 (a->ptype == PRIM_BOOL && b->ptype == PRIM_NULL)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_NULL))
        return NULL;
    object *obj = (object*)c;
    return obj;
}

object *prim_binary_sub(object *this_, object *other)
{
    if (this_->type != OBJ_PRIMITIVE || other->type != OBJ_PRIMITIVE)
        return NULL;

    objprim *a = (objprim*)this_;
    objprim *b = (objprim*)other;
    objprim *c = NULL;

    if (match(a, b, PRIM_DOUBLE)) {
        if ((a->ptype == PRIM_DOUBLE && b->ptype == PRIM_DOUBLE)) {
            c = create_new_primitive(PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) - PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_BOOL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_BOOL)) {
            c = create_new_primitive(PRIM_DOUBLE);
            convert_prim(a, PRIM_DOUBLE);
            convert_prim(b, PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) - PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_STRING && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_NULL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_NULL) ||
                 (a->ptype == PRIM_STRING && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_STRING && b->ptype == PRIM_BOOL) ||
                 (a->ptype == PRIM_BOOL && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_STRING && b->ptype == PRIM_NULL) ||
                 (a->ptype == PRIM_NULL && b->ptype == PRIM_STRING)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_BOOL)) {
        if (a->ptype == PRIM_BOOL && b->ptype == PRIM_BOOL) {
            c = create_new_primitive(PRIM_BOOL);
            PRIM_AS_BOOL(c) = PRIM_AS_BOOL(a) - PRIM_AS_BOOL(b);
        }
        else if ((a->ptype == PRIM_NULL && b->ptype == PRIM_BOOL) ||
                (a->ptype == PRIM_BOOL && b->ptype == PRIM_NULL)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_NULL))
        return NULL;
    object *obj = (object*)c;
    return obj;
}

object *prim_binary_mul(object *this_, object *other)
{
    if (this_->type != OBJ_PRIMITIVE || other->type != OBJ_PRIMITIVE)
        return NULL;

    objprim *a = (objprim*)this_;
    objprim *b = (objprim*)other;
    objprim *c = NULL;

    if (match(a, b, PRIM_DOUBLE)) {
        if ((a->ptype == PRIM_DOUBLE && b->ptype == PRIM_DOUBLE)) {
            c = create_new_primitive(PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) * PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_BOOL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_BOOL)) {
            c = create_new_primitive(PRIM_DOUBLE);
            convert_prim(a, PRIM_DOUBLE);
            convert_prim(b, PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) * PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_STRING && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_STRING)) {
            objprim *basestring = (a->ptype == PRIM_STRING ? a : b);
            int times = (int)(a->ptype == PRIM_DOUBLE ? PRIM_AS_DOUBLE(a) :
                    PRIM_AS_DOUBLE(b));
            c = repeat(basestring, times);
        }
        else if ((a->ptype == PRIM_NULL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_NULL)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_STRING)) {
        if ((a->ptype == PRIM_STRING && b->ptype == PRIM_STRING) ||
            (a->ptype == PRIM_STRING && b->ptype == PRIM_BOOL) ||
            (a->ptype == PRIM_BOOL && b->ptype == PRIM_STRING) ||
            (a->ptype == PRIM_STRING && b->ptype == PRIM_NULL) ||
            (a->ptype == PRIM_NULL && b->ptype == PRIM_STRING)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_BOOL)) {
        if (a->ptype == PRIM_BOOL && b->ptype == PRIM_BOOL) {
            c = create_new_primitive(PRIM_BOOL);
            PRIM_AS_BOOL(c) = PRIM_AS_BOOL(a) * PRIM_AS_BOOL(b);
        }
        else if ((a->ptype == PRIM_NULL && b->ptype == PRIM_BOOL) ||
                 (a->ptype == PRIM_BOOL && b->ptype == PRIM_NULL)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_NULL))
        return NULL;
    object *obj = (object*)c;
    return obj;
}

object *prim_binary_div(object *this_, object *other)
{
    if (this_->type != OBJ_PRIMITIVE || other->type != OBJ_PRIMITIVE)
        return NULL;

    objprim *a = (objprim*)this_;
    objprim *b = (objprim*)other;
    objprim *c = NULL;

    if (match(a, b, PRIM_DOUBLE)) {
        if ((a->ptype == PRIM_DOUBLE && b->ptype == PRIM_DOUBLE)) {
            c = create_new_primitive(PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) / PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_BOOL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_BOOL)) {
            c = create_new_primitive(PRIM_DOUBLE);
            convert_prim(a, PRIM_DOUBLE);
            convert_prim(b, PRIM_DOUBLE);
            PRIM_AS_DOUBLE(c) = PRIM_AS_DOUBLE(a) / PRIM_AS_DOUBLE(b);
        }
        else if ((a->ptype == PRIM_STRING && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_NULL && b->ptype == PRIM_DOUBLE) ||
                 (a->ptype == PRIM_DOUBLE && b->ptype == PRIM_NULL) ||
                 (a->ptype == PRIM_STRING && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_STRING && b->ptype == PRIM_BOOL) ||
                 (a->ptype == PRIM_BOOL && b->ptype == PRIM_STRING) ||
                 (a->ptype == PRIM_STRING && b->ptype == PRIM_NULL) ||
                 (a->ptype == PRIM_NULL && b->ptype == PRIM_STRING)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_BOOL)) {
        if (a->ptype == PRIM_BOOL && b->ptype == PRIM_BOOL) {
            c = create_new_primitive(PRIM_BOOL);
            PRIM_AS_BOOL(c) = PRIM_AS_BOOL(a) / PRIM_AS_BOOL(b);
        }
        else if ((a->ptype == PRIM_NULL && b->ptype == PRIM_BOOL) ||
                 (a->ptype == PRIM_BOOL && b->ptype == PRIM_NULL)) {
            return NULL;
        }
    }
    else if (match(a, b, PRIM_NULL))
        return NULL;
    object *obj = (object*)c;
    return obj;
}
