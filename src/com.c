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
    // out_of_bounds_err: print error and exit(1) when array out of bounds access is detected
    {
        fprintf(f, "out_of_bounds_err:\n");
        fprintf(f, "    mov rax, 1\n");
        fprintf(f, "    mov rdi, 2\n");
        fprintf(f, "    lea rsi, [rel out_of_bounds_msg]\n");
        fprintf(f, "    mov rdx, 34\n");
        fprintf(f, "    syscall\n");
        fprintf(f, "    mov rax, 60\n");
        fprintf(f, "    mov rdi, 1\n");
        fprintf(f, "    syscall\n");
    }
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
          
    // mem: allocate memory and put address in rax
    {   
        fprintf(f, "mem:\n");
        fprintf(f, "    push rdx\n");
        fprintf(f, "    push rcx\n");
        fprintf(f, "    mov rax, 12\n");
        fprintf(f, "    xor rdi, rdi\n");
        fprintf(f, "    syscall\n");
        // rbx = base of allocation (ptr)
        // rcx = stride
        // rdx = size
        fprintf(f, "    mov rbx, rax\n");
        fprintf(f, "    pop rcx\n");
        fprintf(f, "    pop rdx\n");
        // prepare next SYS_BRK call by calculating malloc size
        // end of allocation = base_ptr + sizeof(header) + stride * size
        fprintf(f, "    mov rdi, rcx\n");
        fprintf(f, "    imul rdi, rdx\n");
        fprintf(f, "    add rdi, 16\n");
        fprintf(f, "    add rdi, rbx\n");
        fprintf(f, "    mov rax, 12\n");
        fprintf(f, "    push rcx\n");
        fprintf(f, "    syscall\n");
        // set header to { stride, size} and advance it afterwards, push result to stack
        fprintf(f, "    pop rcx\n");
        fprintf(f, "    mov [rbx], rcx\n");
        fprintf(f, "    mov [rbx + 8], rdx\n");
        fprintf(f, "    lea rax, [rbx + 16]\n");
        fprintf(f, "    ret\n");
    }

    // set_arr_idx: sets a value of an array previously allocated by mem
    {
        fprintf(f, "set_arr_idx:\n");
        // rbx = base of allocation (ptr)
        // rcx = stride 
        // rdx = size
        // rsi = index
        // r8  = value
        fprintf(f, "    mov rcx, [rbx - 16]\n");
        fprintf(f, "    mov rdx, [rbx - 8]\n");
        fprintf(f, "    cmp rsi, rdx\n");  // compare 
        fprintf(f, "    jae out_of_bounds_err\n"); // throw error if out of bounds access
        fprintf(f, "    mov rax, rsi\n");
        fprintf(f, "    imul rax, rcx\n");
        fprintf(f, "    add rax, rbx\n");
        fprintf(f, "    lea r9, [rel set_arr_idx_stride_table]\n");
        fprintf(f, "    jmp [r9 + rcx * 8]\n");
        fprintf(f, "set_arr_idx_stride_table:\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq set_arr_idx_handle_1\n");
        fprintf(f, "    dq set_arr_idx_handle_2\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq set_arr_idx_handle_4\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq set_arr_idx_handle_8\n");
        fprintf(f, "set_arr_idx_handle_1:\n");
        fprintf(f, "    mov [rax], r8b\n");
        fprintf(f, "    jmp set_arr_idx_handle_done\n");
        fprintf(f, "set_arr_idx_handle_2:\n");
        fprintf(f, "    mov [rax], r8w\n");
        fprintf(f, "    jmp set_arr_idx_handle_done\n");
        fprintf(f, "set_arr_idx_handle_4:\n");
        fprintf(f, "    mov [rax], r8d\n");
        fprintf(f, "    jmp set_arr_idx_handle_done\n");
        fprintf(f, "set_arr_idx_handle_8:\n");
        fprintf(f, "    mov [rax], r8\n");
        fprintf(f, "    jmp set_arr_idx_handle_done\n");
        fprintf(f, "set_arr_idx_handle_done:\n");
        fprintf(f, "    ret\n");
    }

    // load: pushes a value at index of an array to the stack
    {

        // rbx = base of allocation (ptr)
        // rcx = stride
        // rdx = size
        // rsi = index
        fprintf(f, "load:\n");
        fprintf(f, "    mov rcx, [rbx - 16]\n");
        fprintf(f, "    mov rdx, [rbx - 8]\n");
        fprintf(f, "    cmp rsi, rdx\n");  // compare 
        fprintf(f, "    jae out_of_bounds_err\n"); // throw error if out of bounds access
        fprintf(f, "    mov rax, rsi\n");
        fprintf(f, "    imul rax, rcx\n");
        fprintf(f, "    add rax, rbx\n");
        fprintf(f, "    lea r9, [rel load_stride_table]\n");
        fprintf(f, "    jmp [r9 + rcx * 8]\n");
        fprintf(f, "load_stride_table:\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq load_handle_1\n");
        fprintf(f, "    dq load_handle_2\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq load_handle_4\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq 0\n");
        fprintf(f, "    dq load_handle_8\n");
        fprintf(f, "load_handle_1:\n");
        fprintf(f, "    movzx rcx, byte [rax]\n");
        fprintf(f, "    jmp load_handle_done\n");
        fprintf(f, "load_handle_2:\n");
        fprintf(f, "    movzx rcx, word [rax]\n");
        fprintf(f, "    jmp load_handle_done\n");
        fprintf(f, "load_handle_4:\n");
        fprintf(f, "    mov eax, dword [rax]\n");
        fprintf(f, "    jmp load_handle_done\n");
        fprintf(f, "load_handle_8:\n");
        fprintf(f, "    mov rcx, [rax]\n");
        fprintf(f, "    jmp load_handle_done\n");
        fprintf(f, "load_handle_done:\n");
        fprintf(f, "    ret\n");
    }
}

void append_bindings(da_t *prog, FILE *f) {
    fprintf(f, "; ----- BINDINGS -----\n");
    fprintf(f, "section .bss\n");
    for (size_t i = 0; i < prog->length; i++) {
        op_t *op = (op_t *)da_get(prog, i);

        if (op->type != OP_LET) continue;
        fprintf(f, "%s resq 1\n", op->name);
    }
}

int compile(da_t *prog) {
    _Static_assert(OP_COUNT == 27,
                   "Exhaustive operator handling inside compile");

    FILE *f = fopen("out.tmp", "w");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fprintf(f, "BITS 64\n");
    fprintf(f, "section .data\n");
    fprintf(f, "div_zero_msg: db \"error: division by zero\", 10\n");
    fprintf(f, "out_of_bounds_msg: db \"error: array out of bounds access\", 10\n");
    append_bindings(prog, f);
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
            fprintf(f, "    pop rax\n");
            fprintf(f, "    xor rbx, rbx\n");
            fprintf(f, "    cmp rax, rbx\n");
            fprintf(f, "    je .ik_caddr_%zu\n", op->jmp_addr);
            break;
        case OP_ELSE:
            fprintf(f, "    ; <OP_ELSE>\n");
            fprintf(f, "    jmp .ik_caddr_%zu\n", op->jmp_addr);
            fprintf(f, ".ik_caddr_%zu:\n", i);
            break;
        case OP_ENDIF:
            fprintf(f, "    ; <OP_END>\n");
            fprintf(f, ".ik_caddr_%zu:\n", i);
            break;
        case OP_WHILE:
            fprintf(f, "    ; <OP_WHILE>\n");
            fprintf(f, ".ik_caddr_%zu:\n", i);
            break;
        case OP_DO:
            fprintf(f, "    ; <OP_DO>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    xor rbx, rbx\n");
            fprintf(f, "    cmp rax, rbx\n");
            fprintf(f, "    je .ik_caddr_%zu\n", op->jmp_addr); 
            break;
        case OP_ENDWHILE:
            fprintf(f, "    ; <OP_ENDWHILE>\n");
            fprintf(f, "    jmp .ik_caddr_%zu\n", op->jmp_addr);
            fprintf(f, ".ik_caddr_%zu:\n", i);
            break;
        case OP_LET:
            fprintf(f, "    ; <OP_LET>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    mov qword [%s], rax\n", op->name);
            break;
        case OP_SET_VALUE:
            fprintf(f, "    ; <OP_SET>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    mov qword [%s], rax\n", op->name);
            break;
        case OP_SET_ARR_IDX:
            fprintf(f, "    ; <OP_SET_ARR_IDX>\n");
            fprintf(f, "    mov rbx, [%s]\n", op->name); // load ptr to memory buffer
            fprintf(f, "    pop rsi\n");
            fprintf(f, "    pop r8\n");
            fprintf(f, "    call set_arr_idx\n");
            break;
        case OP_LOAD:
            fprintf(f, "    ; <OP_LOAD>\n");
            fprintf(f, "    mov rbx, [%s]\n", op->name); // load ptr to memory buffer
            fprintf(f, "    pop rsi\n");
            fprintf(f, "    call load\n");
            fprintf(f, "    push rcx\n");
            break;
        case OP_IDENT:
            break;
        case OP_PUSH_IDENT:
            fprintf(f, "    ; <OP_PUSH_IDENT>\n");
            fprintf(f, "    push qword [%s]\n", op->name);
            break;
        case OP_MEM:
            fprintf(f, "    ; <OP_MEM>\n");
            fprintf(f, "    pop rcx\n");
            fprintf(f, "    pop rdx\n");
            fprintf(f, "    call mem\n");
            fprintf(f, "    push rax\n");
            break;
        default:
            fprintf(stderr, "%s:%zu:%zu: error: unknown Operator `%d` reached.\n",
                    op->location.file, op->location.row, op->location.col, op->type);
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

    if (system("nasm -felf64 -g -F dwarf out.asm -o out.o") != 0) {
        fprintf(stderr, "error: nasm failed\n");
        return 1;
    }
    if (system("ld out.o -o out") != 0) {
        fprintf(stderr, "error: ld failed\n");
        return 1;
    }

    return 0;
}
