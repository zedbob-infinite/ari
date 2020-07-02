#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repl.h"
#include "compile.h"

void run_file(const char* path);
void arimain(int argc, char** argv);

int main(int argc, char** argv)
{
    arimain(argc, argv);

    return 0;
}

void arimain(int argc, char** argv)
{
    if (argc > 1)
    {
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
    
    char *filename = strcat(strtok(pathname, "."), ".aric");
    free(pathname);

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        char* source = read_file(path);
        compile(source);
    }
}
