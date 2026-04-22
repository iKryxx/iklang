#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "da.h"
#include "parse.h"
#include "sim.h"

#define STACK_SIZE 10000000 // 10000000 integers

da_t stack = {0};

void push(long long ival) {
    if (stack.length == STACK_SIZE) {
        fprintf(stderr, "error: stack overflow.");
    }

    da_push(&stack, &ival);
}

int pop() {
    int *value = da_pop(&stack);

    if (value == NULL) {
        fprintf(stderr, "error: stack underflow.");
        exit(1);
    }

    return *value;
}

void sim_run(da_t *prog) {
    assert(OP_COUNT == 6 && "Exhaustive operator handling inside sim_run");

    stack = da_new(long long);

    for (size_t i = 0; i < prog->length; i++) {
        op_t *op = da_get(prog, i);
        switch (op->type) {
        case OP_PUSH_INT:
            push(op->ival);
            break;

        case OP_PLUS: {
            long long b = pop();
            long long a = pop();
            push(a + b);
            break;
        }
        case OP_MINUS: {
            long long b = pop();
            long long a = pop();
            push(a - b);
            break;
        }

        case OP_STAR: {
            long long b = pop();
            long long a = pop();
            push(a * b);
            break;
        }

        case OP_SLASH: {
            long long b = pop();
            long long a = pop();
            push(a / b);
            break;
        }

        case OP_DUMP: {
            long long val = pop();
            printf("%lld\n", val);
            break;
        }
        default:
            fprintf(stderr, "error: unknown Operator\n");
            da_free(&stack);
            exit(1);
        }
    }

    da_free(&stack);
}
