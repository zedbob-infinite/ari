#include <stdio.h>
#include <string.h>

#include "builtin.h"
#include "memory.h"
#include "object.h"

static char *take_string(char *val, int length)
{
    char *buffer = ALLOCATE(char, length +1);
    buffer = memcpy(buffer, val, length);
    buffer[length] = '\0';
    return buffer;
}

object *builtin_println(int argcount, object **args)
{
    for (int i = argcount - 1; i >= 0; i--)
        print_object(args[i]);
    printf("\n");
    return NULL;
}

object *builtin_input(int argcount, object **args)
{
    char buffer[1024];
    int length;

    for (int i = argcount - 1; i >= 0; i--)
        print_object(args[i]);

    fgets(buffer, sizeof(buffer), stdin);

    length = strlen(buffer);
    length[buffer - 1] = '\0';
    objprim *result = create_new_primitive(PRIM_STRING);
    PRIM_AS_STRING(result) = take_string(buffer, length);
    
    return (object*)result;
}

object *builtin_type(int argcount, object **args)
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
    char *msg;
    switch (obj->type) {
        case OBJ_PRIMITIVE:
        {
            msg = "<primitive>";
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
    }
    objprim *result = create_new_primitive(PRIM_STRING);
    PRIM_AS_STRING(result) = take_string(msg, strlen(msg));
    return (object*)result;
}
