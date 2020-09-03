#include <stdio.h>

#include "builtin.h"
#include "memory.h"
#include "objclass.h"
#include "objcode.h"
#include "object.h"
#include "objprim.h"

void init_object(object *obj, objtype type)
{
    obj->type = type;
    obj->next = NULL;
    obj->__attrs__ = NULL;
}

void print_object(object *obj)
{
    if (!obj) {
        printf("No object found.\n");
        return;
    }
	switch (obj->type) {
		case OBJ_PRIMITIVE:
		{
			objprim *prim = (objprim*)obj;
			switch (prim->ptype) {
				case PRIM_INT:
					printf("%d", PRIM_AS_INT(prim));
					break;
				case PRIM_DOUBLE:
					printf("%f", PRIM_AS_DOUBLE(prim));
					break;
				case PRIM_STRING:
					printf("%s", PRIM_AS_STRING(prim));
					break;
				case PRIM_BOOL:
					printf("%s", PRIM_AS_BOOL(prim) ? "true" : "false");
					break;
				case PRIM_NULL:
					printf("null");
					break;
				default:
					break;
			}
			break;
        case OBJ_CODE:
        {
            objcode *codeobj = (objcode*)obj;
            printf("<code object> at %p", codeobj);
            break;
        }
        case OBJ_CLASS:
        {
            objclass *classobj = (objclass*)obj;
            printf("<class object> at %p", classobj);
            break;
        }
        case OBJ_INSTANCE:
        {
            objinstance *instanceobj = (objinstance*)obj;
            char *name = instanceobj->class->name;
            printf("<instance object: %s class> at %p", name, instanceobj);
            break;
        }
        case OBJ_BUILTIN:
        {
            /*objbuiltin *builtin_obj = (objbuiltin*)obj;*/
            printf("<builtin>");
            break;
        }
		default:
			break;
		}
	}
}
