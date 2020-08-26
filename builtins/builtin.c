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
