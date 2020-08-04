#ifndef ari_tokenizer_h
#define ari_tokenizer_h

#include "token.h"

typedef struct scanner_t
{
    const char *start;
    const char *current;
    int line;
    int length;
    int num_tokens;
    int capacity;
    const char *source;
    token **tokens;
} scanner;

void init_scanner(scanner *scan);
void scan_tokens(scanner *scan, const char *source);

#endif
