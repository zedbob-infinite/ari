#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repl.h"

void repl(void)
{
    char* msg = "ari 0.0.1.dev\n[GCC 9.3.0] on linux\n";
    printf("%s", msg);

    char line[1024];
    for (;;) {
        printf(">>> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
    }
}
