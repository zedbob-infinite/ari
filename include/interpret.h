#ifndef ari_interpret_h
#define ari_interpret_h

extern typedef struct VM;

void interpret_line(VM *vm, char *source);

#endif
