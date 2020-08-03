#ifndef ari_compile_h
#define ari_compile_h

#include "vm.h"

typedef struct Compiler_h Compiler;

void compile(VM *vm, const char *source);

#endif
