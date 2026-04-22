#include "com.h"
#include "parse.h"
#include "sim.h"
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

void usage(void) {
    printf("Usage:\n");
    printf("    ./iklc <file>");
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

    // TODO: implement option to allow to toggle simulation mode
    if (false)
        sim_run(&prog);
    compile(&prog);

    return 0;
}
