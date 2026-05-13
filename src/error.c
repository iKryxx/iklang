#include "error.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void err_print(err_type_t type, err_ctx_t ctx) {
    switch (type) {
    case ERR_UNEXPECTED_KEYWORD:
        fprintf(stderr, "%s:%zu:%zu: error: unexpected `%s` keyword.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0]);
        break;
    case ERR_UNCLOSED_BLOCK:
        fprintf(stderr, "%s:%zu:%zu: error: unclosed `if` block.\n",
                ctx.file, ctx.row, ctx.col);
        break;
    case ERR_UNDEFINED_SYMBOL:
        fprintf(stderr, "%s:%zu:%zu: error: undefined symbol `%s`.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0]);
        break;
    case ERR_REDEFINITION:
        fprintf(stderr, "%s:%zu:%zu: error: redefinition of `%s`.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0]);
        break;
    case ERR_IDENTIFIER_TOO_LONG:
        fprintf(stderr, "%s:%zu:%zu: error: identifier `%s` exceeds maximum length of %d.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0], MAX_IDENTIFIER_LENGTH);
        break;
    case ERR_UNCLOSED_STRING_LITERAL:
        fprintf(stderr, "%s:%zu:%zu: error: unclosed string literal.\n",
                ctx.file, ctx.row, ctx.col);
        break;
    case ERR_NESTED_MACRO_DEFINITION:
        fprintf(stderr, "%s:%zu:%zu: error: macros cannot be declared in other macros.\n",
                ctx.file, ctx.row, ctx.col);
        break;
    case ERR_CIRCULAR_INCLUDE:
        fprintf(stderr, "%s:%zu:%zu: error: circular include of `%s`.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0]);
        break;
    case ERR_DOUBLE_INCLUDE:
        fprintf(stderr, "%s:%zu:%zu: error: `%s` has already been included.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0]);
        break;
    case ERR_STACK_UNDERFLOW:
        fprintf(stderr, "%s:%zu:%zu: error: stack underflow when trying to perform operation `%s`.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0]);
        break;
    case ERR_TYPE_MISSMATCH:
        if(ctx.name[1] != NULL) {
            fprintf(stderr, "%s:%zu:%zu: error: couldn't perform operation on ´%s´ and ´%s´.\n",
                    ctx.file, ctx.row, ctx.col, ctx.name[0], ctx.name[1]);
        } else {
            
            fprintf(stderr, "%s:%zu:%zu: error: couldn't perform operation on ´%s´.\n",
                    ctx.file, ctx.row, ctx.col, ctx.name[0]);
        }
        break;
    case ERR_STACK_NOT_EMPTY:
        fprintf(stderr, "%s:%zu:%zu: error: stack must be empty at the end of the program.\n",
                ctx.file, ctx.row, ctx.col);
        break;
    case ERR_BRANCH_STACK_MISMATCH:
        fprintf(stderr, "%s:%zu:%zu: error: `%s` block must not have a net effect on the stack.\n",
                ctx.file, ctx.row, ctx.col, ctx.name[0]);
        break;
    }
}

void err_print_info(const char *fmt, ...) {
    fprintf(stderr, "    - info: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

_Noreturn void err_throw(err_type_t type, err_ctx_t ctx) {
    err_print(type, ctx);
    exit(1);
}
