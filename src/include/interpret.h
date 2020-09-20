#ifndef ari_interpret_h
#define ari_interpret_h

#include "io.h"
#include "vm.h"

void interpret(AriFile *file);
void interpret_line(VM *vm, char *source);

#endif
