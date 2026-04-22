#include "file_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_entire_file(const char *path, char **out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    *out = malloc(fsize + 1);
    if (!*out) {
        perror("malloc");
        exit(1);
    }
    fread(*out, fsize, 1, f);

    fclose(f);
    (*out)[fsize] = '\0';
}

void tokenize_file(const char *text, const char *filename, da_t *out) {
    *out = da_new(text_token_t);

    size_t row = 1, col = 1;
    while (*text) {
        if (*text == ' ') {
            col++;
            text++;
            continue;
        } else if (*text == '\n') {
            row++;
            text++;
            col = 1;
            continue;
        } else {
            text_token_t new_token = {0};

            new_token.file_name = filename;
            new_token.row = row;
            new_token.column = col;
            new_token.token = text;

            const char *wsi = strchr(text, ' ');
            const char *nli = strchr(text, '\n');
            if (wsi == NULL)
                wsi = text + strlen(text);
            if (nli == NULL)
                nli = text + strlen(text);

            const char *mini = wsi < nli ? wsi : nli;
            int token_len = (int)(mini - text);

            new_token.token_len = token_len;
            text += token_len;
            col += token_len;

            da_push(out, &new_token);
        }
    }
}

void print_tokenized_file(const da_t *tokens) {
    for (size_t i = 0; i < tokens->length; i++) {
        text_token_t tok = *(text_token_t *)da_get(tokens, i);

        printf("%s:%llu:%llu: %.*s\n", tok.file_name, tok.row, tok.column,
               tok.token_len, tok.token);
    }
}
