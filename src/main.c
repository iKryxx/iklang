#include "com.h"
#include "parse.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void print_ops(da_t *prog) {
    for (size_t i = 0; i < prog->length; i++) {
        op_t *cur = (op_t *)da_get(prog, i);
        printf("[%zu] - %.15s", i, op_type_name(cur->type));

        switch (cur->type) {

        case OP_PUSH_INT:
            printf("(%llu)", cur->ival);
            break;
        case OP_IF:
        case OP_ENDIF:
        case OP_ELSE:
        case OP_DO:
        case OP_WHILE:
            printf(" (%zu)", cur->jmp_addr);
            break;
        case OP_LET:
        case OP_IDENT:
        case OP_PUSH_IDENT:
            printf(" (%s)", cur->name);
            break;
        default: break;
        }
        printf("\n");
    }
}

void usage(void) {
    printf("Usage:\n");
    printf("    ./iklc <file>\n");
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
    compile(&prog);

    return 0;
}
