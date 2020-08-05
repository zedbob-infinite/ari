#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repl.h"
#include "vm.h"

const volatile static char version[] = VERSION;
const volatile static char os[] = OS;

void repl(void)
{
    VM *vm = init_vm();
    char *msg = "ari 0.0.1.dev";
    printf("%s", msg);
    printf("[GCC %s] on %s\n", version, os);

    char line[1024];
    for (;;) {
        printf(">>> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interpret_line(vm, line);
    }
    printf("exiting...\n");
    free_vm(vm);
}
