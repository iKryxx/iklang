#include "com.h"
#include "parse.h"
#include <assert.h>
#include <stdio.h>

static const char *comp_op_to_mov_instr(op_type_t op) {
    switch (op) {

    case OP_EQUALS:
        return "cmove";
    case OP_GREATER:
        return "cmovg";
    case OP_GREATER_EQUALS:
        return "cmovge";
    case OP_LESS:
        return "cmovl";
    case OP_LESS_EQUALS:
        return "cmovle";
    case OP_NOT_EQUALS:
        return "cmovne";
    default:
        return "";
    }
}

void append_builtins(FILE *f) {
    // div_zero_err: print error and exit(1) when division by zero is detected
    {
        fprintf(f, "div_zero_err:\n");
        fprintf(f, "    mov rax, 1\n");
        fprintf(f, "    mov rdi, 2\n");
        fprintf(f, "    lea rsi, [rel div_zero_msg]\n");
        fprintf(f, "    mov rdx, 24\n");
        fprintf(f, "    syscall\n");
        fprintf(f, "    mov rax, 60\n");
        fprintf(f, "    mov rdi, 1\n");
        fprintf(f, "    syscall\n");
    }

    // Dump: prints whatever is in rdi
    {
        fprintf(f, "; dump(rdi: uint64) -- print rdi as decimal + newline to "
                   "stdout\n");
        fprintf(f, "; in:  rdi\n");
        fprintf(f, "; out: none\n");
        fprintf(f, "; clobbers: rax, rcx, rdx, rdi, rsi, r8, r9\n");
        fprintf(f, "dump:\n");
        fprintf(f, "    mov r9, -3689348814741910323\n");
        fprintf(f, "    sub rsp, 40\n");
        fprintf(f, "    mov BYTE [rsp+31], 10\n");
        fprintf(f, "    lea rcx, [rsp+30]\n");
        fprintf(f, ".L2:\n");
        fprintf(f, "    mov rax, rdi\n");
        fprintf(f, "    lea r8, [rsp+32]\n");
        fprintf(f, "    mul r9\n");
        fprintf(f, "    mov rax, rdi\n");
        fprintf(f, "    sub r8, rcx\n");
        fprintf(f, "    shr rdx, 3\n");
        fprintf(f, "    lea rsi, [rdx+rdx*4]\n");
        fprintf(f, "    add rsi, rsi\n");
        fprintf(f, "    sub rax, rsi\n");
        fprintf(f, "    add eax, 48\n");
        fprintf(f, "    mov BYTE [rcx], al\n");
        fprintf(f, "    mov rax, rdi\n");
        fprintf(f, "    mov rdi, rdx\n");
        fprintf(f, "    mov rdx, rcx\n");
        fprintf(f, "    sub rcx, 1\n");
        fprintf(f, "    cmp rax, 9\n");
        fprintf(f, "    ja .L2\n");
        fprintf(f, "    lea rax, [rsp+32]\n");
        fprintf(f, "    mov edi, 1\n");
        fprintf(f, "    sub rdx, rax\n");
        fprintf(f, "    xor eax, eax\n");
        fprintf(f, "    lea rsi, [rsp+32+rdx]\n");
        fprintf(f, "    mov rdx, r8\n");
        fprintf(f, "    mov rax, 1\n");
        fprintf(f, "    syscall\n");
        fprintf(f, "    add rsp, 40\n");
        fprintf(f, "    ret\n");
    }
}

int compile(da_t *prog) {
    _Static_assert(OP_COUNT == 15,
                   "Exhaustive operator handling inside compile");

    FILE *f = fopen("out.tmp", "w");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fprintf(f, "BITS 64\n");
    fprintf(f, "section .data\n");
    fprintf(f, "div_zero_msg: db \"error: division by zero\", 10\n");
    fprintf(f, "section .text\n");
    append_builtins(f);
    fprintf(f, "global _start\n");
    fprintf(f, "_start:\n");

    for (size_t i = 0; i < prog->length; i++) {
        op_t *op = da_get(prog, i);
        switch (op->type) {

        case OP_PUSH_INT:
            fprintf(f, "    ; <OP_PUSH_INT>\n");
            fprintf(f, "    push %lld\n", op->ival);
            break;
        case OP_PLUS:
            fprintf(f, "    ; <OP_PLUS>\n");
            fprintf(f, "    pop rbx\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    add rax, rbx\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_MINUS:
            fprintf(f, "    ; <OP_MINUS>\n");
            fprintf(f, "    pop rbx\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    sub rax, rbx\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_STAR:
            fprintf(f, "    ; <OP_STAR>\n");
            fprintf(f, "    pop rbx\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    imul rax, rbx\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_SLASH:
            fprintf(f, "    ; <OP_SLASH>\n");
            fprintf(f, "    pop rbx\n");
            fprintf(f, "    test rbx, rbx\n");
            fprintf(f, "    jz div_zero_err\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    cqo\n");
            fprintf(f, "    idiv rbx\n");
            fprintf(f, "    push rax\n");
            break;

        case OP_EQUALS:
            fprintf(f, "    ; <OP_EQUALS>\n");
            goto comparison;
        case OP_GREATER:
            fprintf(f, "    ; <OP_GREATER>\n");
            goto comparison;
        case OP_GREATER_EQUALS:
            fprintf(f, "    ; <OP_GREATER_EQUALS>\n");
            goto comparison;
        case OP_LESS:
            fprintf(f, "    ; <OP_LESS>\n");
            goto comparison;
        case OP_LESS_EQUALS:
            fprintf(f, "    ; <OP_LESS_EQUALS>\n");
            goto comparison;
        case OP_NOT_EQUALS:
            fprintf(f, "    ; <OP_NOT_EQUALS>\n");
            goto comparison;
        comparison:
            fprintf(f, "    xor rcx, rcx\n");
            fprintf(f, "    mov rdx, 1\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    pop rbx\n");
            fprintf(f, "    cmp rbx, rax\n");
            fprintf(f, "    %s rcx, rdx\n", comp_op_to_mov_instr(op->type));
            fprintf(f, "    push rcx\n");
            break;
        case OP_NOT:
            fprintf(f, "    ; <OP_NOT>\n");
            fprintf(f, "    xor rcx, rcx\n");
            fprintf(f, "    mov rdx, 1\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    xor rbx, rbx\n");
            fprintf(f, "    cmp rax, rbx\n");
            fprintf(f, "    cmove rcx, rdx\n");
            fprintf(f, "    push rcx\n");
            break;
        case OP_DUMP:
            fprintf(f, "    ; <OP_DUMP>\n");
            fprintf(f, "    pop rdi\n");
            fprintf(f, "    call dump\n");
            break;
        case OP_DUP:
            fprintf(f, "    ; <OP_DUP>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    push rax\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_IF:
            fprintf(f, "    ; <OP_IF>\n");
            assert(" <OP_IF> is not yet implemented!");
            break;
        default:
            fprintf(stderr, "error: unknown Operator with value %d reached\n",
                    op->type);
            fclose(f);
            remove("out.tmp");
            return 1;
        }
    }

    fprintf(f, ".ik_addr_%zu:\n", prog->length);
    fprintf(f, "    mov rax, 60\n");
    fprintf(f, "    mov rdi, 0\n");
    fprintf(f, "    syscall\n");

    fclose(f);
    rename("out.tmp", "out.asm");

    if (system("nasm -felf64 out.asm -o out.o") != 0) {
        fprintf(stderr, "error: nasm failed\n");
        return 1;
    }
    if (system("ld out.o -o out") != 0) {
        fprintf(stderr, "error: ld failed\n");
        return 1;
    }

    return 0;
}
