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

    if (simulate)
        sim_run(&prog);
    else
        compile(&prog);

    return 0;
}
