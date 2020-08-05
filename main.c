#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
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

static char* read_file(const char* path) 
{                        
  FILE* file = fopen(path, "rb");

  fseek(file, 0L, SEEK_END);                                     
  size_t size = ftell(file);                                 
  rewind(file);                                                  

  char* buffer = malloc(size + 1);                    
  size_t bytes_read = fread(buffer, sizeof(char), size, file);
  buffer[bytes_read] = '\0';                                      

  fclose(file);                                                  
  return buffer;                                                 
}   

void run_file(const char* path)
{
    char *pathname = malloc(sizeof(strlen(path)));
    strcpy(pathname, path);
    
    //strcat(strtok(pathname, "."), ".aric");

    FILE* file = fopen(pathname, "rb");
    if (!file) {
        char* source = read_file(path);
		interpret(source);
    }
}
