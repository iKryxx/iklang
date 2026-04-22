#ifndef PARSER_H
#define PARSER_H

#include "da.h"
#define MAX_OPS 1024

typedef enum {
    OP_PUSH_INT,
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_DUMP,
    OP_COUNT
} op_type_t;

typedef struct {
    op_type_t type;
    int ival;
} op_t;

void parse(da_t *prog, const char *src);

#endif
