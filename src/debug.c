#include <stdio.h>

#include "debug.h"
#include "opcode.h"


void print_bytecode(uint8_t bytecode)
{
    char *msg = NULL;
    switch (bytecode) {
        case OP_POP_FRAME:
            msg = "POP_FRAME";
            break;
        case OP_PUSH_FRAME:
            msg ="PUSH_FRAME";
            break;
        case OP_JMP_LOC:
            msg = "JMP_LOC";
            break;
        case OP_JMP_AFTER:
            msg = "JMP_AFTER";
            break;
        case OP_JMP_FALSE:
            msg = "JMP_FALSE";
            break;
        case OP_POP:
            msg = "POP";
            break;
        case OP_LOAD_CONSTANT:
            msg = "LOAD_CONSTANT";
            break;
        case OP_LOAD_NAME:
            msg = "LOAD_NAME";
            break;
        case OP_LOAD_METHOD:
            msg = "LOAD_METHOD";
            break;
        case OP_CALL_FUNCTION:
            msg = "CALL_FUNCTION";
            break;
        case OP_MAKE_FUNCTION:
            msg = "MAKE_FUNCTION";
            break;
        case OP_CALL_METHOD:
            msg = "CALL_METHOD";
            break;
        case OP_MAKE_METHOD:
            msg = "MAKE_METHOD";
            break;
        case OP_MAKE_CLASS:
            msg = "MAKE_CLASS";
            break;
        case OP_SET_PROPERTY:
            msg = "SET_PROPERTY";
            break;
        case OP_GET_SOURCE:
            msg = "GET_SOURCE";
            break;
        case OP_GET_PROPERTY:
            msg = "GET_PROPERTY";
            break;
        case OP_STORE_NAME:
            msg = "STORE_NAME";
            break;
        case OP_COMPARE:
            msg = "COMPARE";
            break;
        case OP_BINARY_ADD:
            msg = "BINARY_ADD";
            break;
        case OP_BINARY_SUB:
            msg = "BINARY_SUB";
            break;
        case OP_BINARY_MULT:
            msg = "BINARY_MULT";
            break;
        case OP_BINARY_DIVIDE:
            msg = "BINARY_DIVIDE";
            break;
        case OP_NEGATE:
            msg = "NEGATE";
            break;
        case OP_RETURN:
            msg = "RETURN";
            break;
    }
    printf("%-20s", msg);
}
