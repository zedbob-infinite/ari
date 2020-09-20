#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "io.h"
#include "interpret.h"
#include "memory.h"
#include "object.h"
#include "repl.h"
#include "vm.h"

void run_file(const char* path);
void arimain(int argc, char** argv);

int main(int argc, char** argv)
{
    arimain(argc, argv);

    return EXIT_SUCCESS;
}

void arimain(int argc, char** argv)
{
    if (argc > 1) {
        run_file(argv[1]);
    }
    else
        repl();
}

void run_file(const char* path)
{
    AriFile *newfile = get_file(path);
    if (newfile->source) {
        interpret(newfile);
        //FREE(char, source);
    }
    else {
        printf("Could not read file %s\n", path);
        exit(EXIT_FAILURE);
    }
}
