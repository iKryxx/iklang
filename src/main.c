#include "com.h"
#include "parse.h"
#include "sim.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void usage(void) {
    printf("Usage:\n");
    printf("    ./iklc [-s] <file>\n");
    printf("    -s  Run in simulation (interpreter) mode instead of compiling\n");
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
    bool simulate = false;
    da_t prog = {0};

    if (argc < 2 || argc > 3) {
        usage();
        return 1;
    }

    int file_arg = 1;
    if (argc == 3) {
        if (strcmp(argv[1], "-s") == 0) {
            simulate = true;
            file_arg = 2;
        } else {
            usage();
            return 1;
        }
    }

    file = argv[file_arg];
    if (access(file, F_OK) != 0) {
        fprintf(stderr, "error: file `%s` does not exist\n", file);
        return 1;
    }

    parse(&prog, file);

    if (false)
        print_tokens(&prog);

    if (simulate)
        sim_run(&prog);
    else
        compile(&prog);

    return 0;
}
