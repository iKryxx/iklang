#include "com.h"
#include "parse.h"
#include "sim.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void usage(void) {
    printf("Usage:\n");
    printf("    ./iklc <file>\n");
}

void print_tokens(da_t *prog) {
    for (size_t i = 0; i < prog->length; i++) {
        op_t *op = da_get(prog, i);

        if (op->type == OP_PUSH_INT || op->type == OP_IF || op->type == OP_END || op->type == OP_ELSE) {
            printf("%zu %s %llu\n", i, op_type_name(op->type), op->ival);
        } else {
            printf("%zu %s\n", i, op_type_name(op->type));
        }
    }
}

int main(int argc, char **argv) {
    const char *file = NULL;
    da_t prog = {0};

    if (argc != 2) {
        usage();
        return 1;
    }

    file = argv[1];
    if (access(file, F_OK) != 0) {
        fprintf(stderr, "error: file `%s` does not exist\n", file);
        return 1;
    }

    parse(&prog, file);

    return 0;
}
