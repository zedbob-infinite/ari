#ifndef ari_error_h
#define ari_error_h

#include <stdarg.h>
#include <stdint.h>

#include "objstack.h"
#include "vm.h"

intrpstate runtime_error(VM *vm, objstack *stack, size_t line, 
        const char *format, ...);
intrpstate runtime_error_loadname(VM *vm, char *name, uint64_t current);
intrpstate runtime_error_unsupported_operation(VM *vm, uint64_t current, 
        char optype);
intrpstate runtime_error_zero_div(VM *vm, uint64_t current);

#endif
