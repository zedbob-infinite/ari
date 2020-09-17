#include <stdio.h>
#include <string.h>

#include "builtin.h"
#include "memory.h"
#include "object.h"
#include "objprim.h"
#include "objstack.h"
#include "vm.h"



static void construct_msg(objprim *primobj, char *_string_)
{
    int length = strlen(_string_);
    uint32_t hash = hashkey(_string_, length);
    char *takenstring = ALLOCATE(char, length + 1);
    memcpy(takenstring, _string_, length);
    takenstring[length] = '\0';

    PRIM_AS_STRING(primobj) = init_primstring(length, hash, takenstring);
}

object *call_builtin(VM *vm, object *obj, int argcount, 
        object **arguments)
{
    objbuiltin *builtinobj = (objbuiltin*)obj;
    object *result = builtinobj->func(vm, argcount, arguments);
    return result;
}

object *load_builtin(VM *vm, char *name, builtin function)
{
    objbuiltin *builtin_obj = ALLOCATE(objbuiltin, 1);
    init_object(&builtin_obj->header, OBJ_BUILTIN);
    builtin_obj->func = function;

    frame *global = &vm->global.local;
    primstring *newname = create_primstring(name);
    objhash_set(&global->locals, newname, (object*)builtin_obj);
    return (object*)builtin_obj;
}

object *builtin_println(VM *vm, int argcount, object **args)
{
    for (int i = argcount - 1; i >= 0; i--)
        print_object(args[i]);
    printf("\n");
    return NULL;
}

object *builtin_input(VM *vm, int argcount, object **args)
{
    char buffer[1024];

    for (int i = argcount - 1; i >= 0; i--)
        print_object(args[i]);

    char *check = fgets(buffer, sizeof(buffer), stdin);

    if (!check)
        return NULL;

    primstring *newstring = create_primstring(buffer);
    objprim *result = create_new_primitive(PRIM_STRING);
    PRIM_AS_STRING(result) = newstring;
    
    return (object*)result;
}

object *builtin_type(VM *vm, int argcount, object **args)
{
    object *obj = args[0];
    if (argcount > 1) {
        printf("type() takes only one argument.\n");
        return NULL;
    }
    else if (argcount == 0) {
        printf("type() takes one argument.\n");
        return NULL;
    }
    char *msg = NULL;
    switch (obj->type) {
        case OBJ_PRIMITIVE:
        {
			objprim *primobj = (objprim*)obj;
			switch (primobj->ptype) {
				case PRIM_DOUBLE:
					msg = "<double>";
					break;
				case PRIM_STRING:
					msg = "<string>";
					break;
				case PRIM_BOOL:
					msg = "<bool>";
					break;
				case PRIM_NULL:
					msg = "<null>";
					break;
                default:
                    msg = "<unknown primitive type>";
                    break;
			}
            break;
        }
        case OBJ_CLASS:
            msg = "<class>";
            break;
        case OBJ_INSTANCE:
            msg = "<instance>";
            break;
        case OBJ_MODULE:
            msg = "<module>";
            break;
        case OBJ_CODE:
            msg = "<code>";
            break;
        case OBJ_BUILTIN:
            msg = "<builtin>";
            break;
        default:
            msg = "<unknown object type>";
            break;
    }
    objprim *result = create_new_primitive(PRIM_STRING);
    construct_msg(result, msg);
    return (object*)result;
}
