#ifndef PARSE_H
#define PARSE_H

#define MAX_IDENTIFIER_LENGTH 16

#include "da.h"

typedef enum {
    OP_PUSH_INT,
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_EQUALS,
    OP_GREATER,
    OP_GREATER_EQUALS,
    OP_LESS,
    OP_LESS_EQUALS,
    OP_NOT,
    OP_NOT_EQUALS,
    OP_DUMP,
    OP_DUP,
    OP_IF,
    OP_ENDIF,
    OP_ELSE,
    OP_DO,
    OP_WHILE,
    OP_ENDWHILE,
    OP_LET,
    OP_SET_VALUE,
    OP_SET_ARR_IDX,
    OP_IDENT,
    OP_PUSH_IDENT,
    OP_MEM,
    OP_LOAD,
    OP_STRING_LITERAL,
    OP_SYSCALL3,
    OP_MACRO,
    OP_ENDMACRO,
    OP_COUNT
} op_type_t;

typedef struct {
    op_type_t type;
    char name[MAX_IDENTIFIER_LENGTH + 1];
    union {
        long long ival;
        size_t jmp_addr;
        char *str_literal;
    };
    struct {
        const char *file;
        size_t row;
        size_t col;
    } location;
} op_t;

void parse(da_t *prog, const char *src);

const char *op_type_name(op_type_t o);

op_type_t op_name_type(const char *name);

#endif
