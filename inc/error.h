#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>

typedef enum {
    ERR_UNEXPECTED_KEYWORD,
    ERR_UNCLOSED_BLOCK,
    ERR_UNDEFINED_SYMBOL,
    ERR_REDEFINITION,
    ERR_IDENTIFIER_TOO_LONG,
} err_type_t;

typedef struct {
    const char *file;
    size_t row;
    size_t col;
    const char *name;
} err_ctx_t;

#define ERR_CTX(loc, sym) ((err_ctx_t){ (loc).file, (loc).row, (loc).col, (sym) })

void err_print(err_type_t type, err_ctx_t ctx);
_Noreturn void err_throw(err_type_t type, err_ctx_t ctx);

#endif
