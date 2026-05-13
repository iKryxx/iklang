#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>

typedef enum {
    ERR_UNEXPECTED_KEYWORD,
    ERR_UNCLOSED_BLOCK,
    ERR_UNDEFINED_SYMBOL,
    ERR_REDEFINITION,
    ERR_IDENTIFIER_TOO_LONG,
    ERR_UNCLOSED_STRING_LITERAL,
    ERR_NESTED_MACRO_DEFINITION,
    ERR_CIRCULAR_INCLUDE,
    ERR_DOUBLE_INCLUDE,
    ERR_STACK_UNDERFLOW,
    ERR_TYPE_MISSMATCH,
    ERR_STACK_NOT_EMPTY,
    ERR_BRANCH_STACK_MISMATCH
} err_type_t;

typedef struct {
    const char *file;
    size_t row;
    size_t col;
    const char *name[2];
} err_ctx_t;

#define ERR_CTX(loc, sym) ((err_ctx_t){ (loc).file, (loc).row, (loc).col, {(sym), NULL} })
#define ERR_CTX2(loc, sym1, sym2) ((err_ctx_t){ (loc).file, (loc).row, (loc).col, {(sym1), (sym2)} })

void err_print(err_type_t type, err_ctx_t ctx);
void err_print_info(const char *fmt, ...);
_Noreturn void err_throw(err_type_t type, err_ctx_t ctx);

#endif
