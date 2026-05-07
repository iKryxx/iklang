/*
 * TODO: push imm in x86-64 only sign-extends a 32-bit immediate, so values above 2^31−1 get silently truncated before they ever reach the store. 
 * Notable in set_arr_idx. 
 */


#include "com.h"
#include "parse.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *comp_op_to_set_instr(op_type_t op) {
    switch (op) {
    case OP_EQUALS:        return "sete";
    case OP_GREATER:       return "setg";
    case OP_GREATER_EQUALS:return "setge";
    case OP_LESS:          return "setl";
    case OP_LESS_EQUALS:   return "setle";
    case OP_NOT_EQUALS:    return "setne";
    default:               return "";
    }
}

static size_t unescape_byte_count(const char *str) {
    size_t len = 0;
    size_t src_len = strlen(str);
    for (size_t i = 0; i < src_len; i++) {
        if (str[i] == '\\' && i + 1 < src_len) {
            switch (str[i + 1]) {
                case 'n': case 't': case 'r': case '0':
                case '\\': case '"':
                    len++; i++; break;
                default:
                    len++; break;
            }
        } else {
            len++;
        }
    }
    return len;
}

char* unescape_to_nasm(char *str) {
    const char *src = str;
    size_t src_len = strlen(src);

    /* worst case: every char becomes ", 255" (6 bytes) + ", 0" + null */
    char *dst = malloc(src_len * 6 + 7);
    if (!dst) perror("malloc");

    size_t w = 0;
    int in_str = 0;

    #define OPEN_STR()   do { if (!in_str) { dst[w++] = '"'; in_str = 1; } } while(0)
    #define CLOSE_STR()  do { if (in_str)  { dst[w++] = '"'; in_str = 0; } } while(0)
    #define EMIT_BYTE(b) do {                                       \
        CLOSE_STR();                                                \
        if(dst[w - 2] == ',')                                       \
            w += sprintf(&dst[w], "%d, ", (unsigned char)(b));      \
        else                                                        \
            w += sprintf(&dst[w], ", %d, ", (unsigned char)(b));    \
    } while(0)

    OPEN_STR();

    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '\\' && i + 1 < src_len) {
            switch (src[i + 1]) {
                case 'n':  EMIT_BYTE(10);   i++; break;
                case 't':  EMIT_BYTE(9);    i++; break;
                case 'r':  EMIT_BYTE(13);   i++; break;
                case '0':  EMIT_BYTE(0);    i++; break;
                case '\\': EMIT_BYTE('\\'); i++; break;
                case '"':  EMIT_BYTE('"');  i++; break;
                default:
                    OPEN_STR();
                    dst[w++] = src[i]; /* keep backslash for unknown escapes */
                    break;
            }
        } else {
            OPEN_STR();
            dst[w++] = src[i];
        }
    }

    CLOSE_STR();
    if(dst[w - 2] == ',')
        dst[w - 2] = '\0';
    else
        dst[w] = '\0';

    #undef OPEN_STR
    #undef CLOSE_STR
    #undef EMIT_BYTE

    return dst;
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

    // dump(rdi) -- print rdi as decimal + newline to stdout
    // in:  rdi
    // out: none
    // clobbers: rax, rcx, rdx, rdi, rsi, r8, r9
    {
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

    // mem(rcx: stride, rdx: size) -- brk-allocate stride*size bytes with a 16-byte header
    // in:  rcx = stride, rdx = size
    // out: rax = pointer past the header
    // clobbers: rax, rcx, rdi, r10, r11
    // rdx is NOT clobbered by syscall, so size survives both brk calls unprotected.
    // rcx IS clobbered by syscall, so stride is saved/restored around each call.
    // r10 holds the base pointer across the second brk call (not clobbered by syscall).
    {
        fprintf(f, "mem:\n");
        fprintf(f, "    push rcx\n");          // save stride (syscall clobbers rcx)
        fprintf(f, "    mov rax, 12\n");
        fprintf(f, "    xor rdi, rdi\n");
        fprintf(f, "    syscall\n");            // rax = current brk; rdx (size) survives
        fprintf(f, "    mov r10, rax\n");       // r10 = base ptr (not clobbered by syscall)
        fprintf(f, "    pop rcx\n");            // restore stride
        fprintf(f, "    mov rdi, rcx\n");
        fprintf(f, "    imul rdi, rdx\n");
        fprintf(f, "    add rdi, 16\n");
        fprintf(f, "    add rdi, r10\n");
        fprintf(f, "    push rcx\n");           // save stride again (next syscall clobbers rcx)
        fprintf(f, "    mov rax, 12\n");
        fprintf(f, "    syscall\n");            // expand brk; r10 and rdx survive
        fprintf(f, "    pop rcx\n");            // restore stride
        fprintf(f, "    mov [r10], rcx\n");     // header: stride
        fprintf(f, "    mov [r10 + 8], rdx\n"); // header: size
        fprintf(f, "    lea rax, [r10 + 16]\n");
        fprintf(f, "    ret\n");
    }

    // set_arr_idx(r10: ptr, rsi: index, r8: value) -- write value at index into array
    // in:  r10 = array pointer (past header), rsi = index, r8 = value
    // out: none
    // clobbers: rax, rcx, rdx, r9
    {
        fprintf(f, "set_arr_idx:\n");
        fprintf(f, "    mov rcx, [r10 - 16]\n");  // stride from header
        fprintf(f, "    mov rdx, [r10 - 8]\n");   // size from header
        fprintf(f, "    cmp rsi, rdx\n");
        fprintf(f, "    jae out_of_bounds_err\n");
        fprintf(f, "    mov rax, rsi\n");
        fprintf(f, "    imul rax, rcx\n");
        fprintf(f, "    add rax, r10\n");
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

    // load(r10: ptr, rsi: index) -- read value at index from array
    // in:  r10 = array pointer (past header), rsi = index
    // out: rax = loaded value (zero-extended)
    // clobbers: rcx, rdx, r9
    {
        fprintf(f, "load:\n");
        fprintf(f, "    mov rcx, [r10 - 16]\n");  // stride from header
        fprintf(f, "    mov rdx, [r10 - 8]\n");   // size from header
        fprintf(f, "    cmp rsi, rdx\n");
        fprintf(f, "    jae out_of_bounds_err\n");
        fprintf(f, "    mov rax, rsi\n");
        fprintf(f, "    imul rax, rcx\n");
        fprintf(f, "    add rax, r10\n");          // rax = address of element
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
        fprintf(f, "    movzx rax, byte [rax]\n");
        fprintf(f, "    jmp load_handle_done\n");
        fprintf(f, "load_handle_2:\n");
        fprintf(f, "    movzx rax, word [rax]\n");
        fprintf(f, "    jmp load_handle_done\n");
        fprintf(f, "load_handle_4:\n");
        fprintf(f, "    mov eax, dword [rax]\n");
        fprintf(f, "    jmp load_handle_done\n");
        fprintf(f, "load_handle_8:\n");
        fprintf(f, "    mov rax, [rax]\n");
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
        fprintf(f, "res_%s resq 1\n", op->name);
    }
}

void append_strings(da_t *prog, FILE *f) {
    fprintf(f, "; ----- STRINGS -----\n");
    for(size_t i = 0; i < prog->length; i++) {
        op_t *op = (op_t*)da_get(prog, i);

        if(op->type != OP_STRING_LITERAL) continue;
        
        char *nasm_str = unescape_to_nasm(op->str_literal);
        fprintf(f, "strlit_%zu: db %s\n", i, nasm_str);
        free(nasm_str);
    }
}

int compile(da_t *prog) {
    _Static_assert(OP_COUNT == 32,
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
    append_strings(prog, f);
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
            fprintf(f, "    pop rcx\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    add rax, rcx\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_MINUS:
            fprintf(f, "    ; <OP_MINUS>\n");
            fprintf(f, "    pop rcx\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    sub rax, rcx\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_STAR:
            fprintf(f, "    ; <OP_STAR>\n");
            fprintf(f, "    pop rcx\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    imul rax, rcx\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_SLASH:
            fprintf(f, "    ; <OP_SLASH>\n");
            fprintf(f, "    pop rcx\n");
            fprintf(f, "    test rcx, rcx\n");
            fprintf(f, "    jz div_zero_err\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    cqo\n");
            fprintf(f, "    idiv rcx\n");
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
            fprintf(f, "    pop rcx\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    cmp rax, rcx\n");
            fprintf(f, "    %s al\n", comp_op_to_set_instr(op->type));
            fprintf(f, "    movzx rax, al\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_NOT:
            fprintf(f, "    ; <OP_NOT>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    test rax, rax\n");
            fprintf(f, "    sete al\n");
            fprintf(f, "    movzx rax, al\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_DUMP:
            fprintf(f, "    ; <OP_DUMP>\n");
            fprintf(f, "    pop rdi\n");
            fprintf(f, "    call dump\n");
            break;
        case OP_DUP:
            fprintf(f, "    ; <OP_DUP>\n");
            fprintf(f, "    push qword [rsp]\n");
            break;
        case OP_IF:
            fprintf(f, "    ; <OP_IF>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    test rax, rax\n");
            fprintf(f, "    jz .ik_caddr_%zu\n", op->jmp_addr);
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
            fprintf(f, "    test rax, rax\n");
            fprintf(f, "    jz .ik_caddr_%zu\n", op->jmp_addr);
            break;
        case OP_ENDWHILE:
            fprintf(f, "    ; <OP_ENDWHILE>\n");
            fprintf(f, "    jmp .ik_caddr_%zu\n", op->jmp_addr);
            fprintf(f, ".ik_caddr_%zu:\n", i);
            break;
        case OP_LET:
            fprintf(f, "    ; <OP_LET>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    mov qword [res_%s], rax\n", op->name);
            break;
        case OP_SET_VALUE:
            fprintf(f, "    ; <OP_SET>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    mov qword [res_%s], rax\n", op->name);
            break;
        case OP_SET_ARR_IDX:
            fprintf(f, "    ; <OP_SET_ARR_IDX>\n");
            fprintf(f, "    mov r10, [res_%s]\n", op->name);  // array base ptr
            fprintf(f, "    pop rsi\n");                   // index
            fprintf(f, "    pop r8\n");                    // value
            fprintf(f, "    call set_arr_idx\n");
            break;
        case OP_LOAD:
            fprintf(f, "    ; <OP_LOAD>\n");
            fprintf(f, "    mov r10, [res_%s]\n", op->name);  // array base ptr
            fprintf(f, "    pop rsi\n");                   // index
            fprintf(f, "    call load\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_IDENT:
            break;
        case OP_PUSH_IDENT:
            fprintf(f, "    ; <OP_PUSH_IDENT>\n");
            fprintf(f, "    push qword [res_%s]\n", op->name);
            break;
        case OP_MEM:
            fprintf(f, "    ; <OP_MEM>\n");
            fprintf(f, "    pop rcx\n");    // stride (top of stack)
            fprintf(f, "    pop rdx\n");    // size
            fprintf(f, "    call mem\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_STRING_LITERAL:
            fprintf(f, "    ; <OP_STRING_LITERAL>\n");
            fprintf(f, "    mov rdx, %zu\n", unescape_byte_count(op->str_literal));
            fprintf(f, "    push rdx\n");
            fprintf(f, "    push strlit_%zu\n", i);
            break;
        case OP_SYSCALL3:
            fprintf(f, "    ; <OP_SYSCALL3>\n");
            fprintf(f, "    pop rax\n");
            fprintf(f, "    pop rdi\n");
            fprintf(f, "    pop rsi\n");
            fprintf(f, "    pop rdx\n");
            fprintf(f, "    syscall\n");
            fprintf(f, "    push rax\n");
            break;
        case OP_MACRO:
            fprintf(stderr, "critical error: unreachable reached inside compile()\n");
            exit(1);
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
