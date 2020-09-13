#ifndef ari_interpret_h
#define ari_interpret_h

#include "vm.h"

void interpret(char *source);
void interpret_line(VM *vm, char *source);

#endif
