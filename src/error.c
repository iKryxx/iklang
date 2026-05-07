#include "error.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

void err_print(err_type_t type, err_ctx_t ctx) {
    switch (type) {
    case ERR_UNEXPECTED_KEYWORD:
        fprintf(stderr, "%s:%zu:%zu: error: unexpected `%s` keyword.\n",
                ctx.file, ctx.row, ctx.col, ctx.name);
        break;
    case ERR_UNCLOSED_BLOCK:
        fprintf(stderr, "%s:%zu:%zu: error: unclosed `if` block.\n",
                ctx.file, ctx.row, ctx.col);
        break;
    case ERR_UNDEFINED_SYMBOL:
        fprintf(stderr, "%s:%zu:%zu: error: undefined symbol `%s`.\n",
                ctx.file, ctx.row, ctx.col, ctx.name);
        break;
    case ERR_REDEFINITION:
        fprintf(stderr, "%s:%zu:%zu: error: redefinition of `%s`.\n",
                ctx.file, ctx.row, ctx.col, ctx.name);
        break;
    case ERR_IDENTIFIER_TOO_LONG:
        fprintf(stderr, "%s:%zu:%zu: error: identifier `%s` exceeds maximum length of %d.\n",
                ctx.file, ctx.row, ctx.col, ctx.name, MAX_IDENTIFIER_LENGTH);
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
                ctx.file, ctx.row, ctx.col, ctx.name);
        break;
    case ERR_DOUBLE_INCLUDE:
        fprintf(stderr, "%s:%zu:%zu: error: `%s` has already been included.\n",
                ctx.file, ctx.row, ctx.col, ctx.name);
        break;
    }
}

_Noreturn void err_throw(err_type_t type, err_ctx_t ctx) {
    err_print(type, ctx);
    exit(1);
}
