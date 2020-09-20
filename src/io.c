#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "memory.h"

/* Stole this code from GNU basename */
static char *Ari_basename(const char *filepath)
{
    char *p = strrchr(filepath, '/');
    return p ? p + 1 : (char *)filepath;
}

static char *Ari_get_ext(const char *filename)
{
    char *p = strrchr(filename, '.');
    return p ? p + 1 : (char *)filename;
}

static char *Ari_get_root(const char *filename)
{
    size_t length = strlen(filename);
    char sepdot[] = ".";
    char sepslash[] = "/";
    char *copy = ALLOCATE(char, length);
    
    if (!copy)
        return NULL;
    memcpy(copy, filename, length);

    size_t allocsize = 50;
    size_t nstringlen = 0;
    
    char *previous = ALLOCATE(char, allocsize);
    if (!previous)
        return NULL;

    char *p = strtok(copy, sepdot);
    if (p) {
        char *s = strtok(copy, sepslash);
        if (s) {
            while (s) {
                nstringlen = strlen(s);
                if (nstringlen > allocsize) {
                    allocsize *= 2;
                    free(previous);
                    previous = ALLOCATE(char, allocsize);
                }
                memset(previous, 0, allocsize);
                memcpy(previous, s, nstringlen);
                s = strtok(NULL, sepslash);
            }
        }
    }
    FREE(char, copy);
    return previous;
}

static char* read_file(const char* path) 
{                        
  FILE* file = fopen(path, "rb");
  
  if (!file)
      return NULL;

  fseek(file, 0L, SEEK_END);                                     
  size_t size = ftell(file);                                 
  rewind(file);                                                  

  char *buffer = NULL;
  buffer = ALLOCATE(char, size + 1);
  size_t bytes_read = fread(buffer, sizeof(char), size, file);
  buffer[bytes_read] = '\0';                                      

  fclose(file);                                                  
  return buffer;                                                 
}   

AriFile *get_file(const char *filepath)
{
    AriFile *newfile = ALLOCATE(AriFile, 1);
    newfile->path = filepath;
    newfile->filename = Ari_basename(filepath);
    newfile->rootname = Ari_get_root(newfile->filename);
    newfile->extname = Ari_get_ext(newfile->filename);
    newfile->source = read_file(filepath);
    return newfile;
}
