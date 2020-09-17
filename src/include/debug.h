#ifndef ari_debug_h
#define ari_debug_h

#include <stdint.h>

#define DEBUG_ARI
#define DEBUG_TOKENIZER
#define DEBUG_ARI_PARSER

#undef DEBUG_TOKENIZER
#undef DEBUG_ARI_PARSER

void print_bytecode(uint8_t bytecode);

#endif
