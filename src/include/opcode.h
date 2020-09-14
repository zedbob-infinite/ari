#ifndef ari_opcode_h
#define ari_opcode_h

typedef enum
{
    OP_LOOP,
    OP_JMP_LOC,
    OP_JMP_AFTER,
    OP_JMP_FALSE,
    OP_POP,
    OP_PUSH_FRAME,
    OP_POP_FRAME,
    OP_LOAD_CONSTANT,
    OP_LOAD_NAME,
    OP_LOAD_METHOD,
    OP_CALL_FUNCTION,
    OP_MAKE_FUNCTION,
    OP_CALL_METHOD,
    OP_MAKE_METHOD,
    OP_MAKE_CLASS,
    OP_SET_PROPERTY,
    OP_GET_PROPERTY,
    OP_STORE_NAME,
    OP_COMPARE,
    OP_BINARY_ADD,
    OP_BINARY_SUB,
    OP_BINARY_MULT,
    OP_BINARY_DIVIDE,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN
} opcode;

#endif