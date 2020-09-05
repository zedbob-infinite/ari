#ifndef ari_object_h
#define ari_object_h

#include "objhash.h"

#define OBJ_IS_PRIMITIVE(obj)   (obj->type == OBJ_PRIMITIVE)
#define OBJ_IS_CLASS(obj)       (obj->type == OBJ_CLASS)
#define OBJ_IS_INSTANCE(obj)    (obj->type == OBJ_INSTANCE)
#define OBJ_IS_MODULE(obj)      (obj->type == OBJ_MODULE)
#define OBJ_IS_CODE(obj)        (obj->type == OBJ_CODE)
#define OBJ_IS_BUILTIN(obj)     (obj->type == OBJ_BUILTIN)

struct object_t;

typedef struct object_t *(*slot)(struct object_t *this_, struct object_t *other);

typedef enum
{
	OBJ_PRIMITIVE,
	OBJ_CLASS,
	OBJ_INSTANCE,
    OBJ_MODULE,
	OBJ_CODE,
    OBJ_BUILTIN,
} objtype;

typedef struct object_t
{
	objtype type;
    struct object_t* next;
    objhash *__attrs__;
    slot __add__;
} object;

void init_object(object *obj, objtype type);
void print_object(object *obj);

#endif
