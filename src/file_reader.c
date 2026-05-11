#include "file_reader.h"
#include "error.h"
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

            new_token.file = filename;
            new_token.row = row;
            new_token.col = col;
            new_token.token = text;

            if(*text == '"') {
                const char *qti = strchr(text + 1, '"');
                const char *nli = strchr(text + 1, '\n');
                if(qti == NULL) 
                    err_throw(ERR_UNCLOSED_STRING_LITERAL, ERR_CTX(new_token, ""));
                if(nli != NULL && nli < qti) {
                    err_throw(ERR_UNCLOSED_STRING_LITERAL, ERR_CTX(new_token, ""));
                }

                int token_len = (int)(qti - text);
                new_token.token_len = token_len + 1;
                text += token_len + 1;
                col += token_len + 1;

                da_push(out, &new_token);
                continue;
            }

            if (*text == ';') {
                const char *nli = strchr(text, '\n');
                if(nli == NULL)
                    nli = text + strlen(text);
                
                int token_len = (int)(nli - text);
                text += token_len;
                col += token_len;
                continue;
            }

            const char *wsi = strchr(text, ' ');
            const char *nli = strchr(text, '\n');
            const char *sci = strchr(text, ';');
            if (wsi == NULL)
                wsi = text + strlen(text);
            if (nli == NULL)
                nli = text + strlen(text);
            if (sci == NULL)
                sci = text + strlen(text);

            const char *mini = wsi < nli ? wsi : nli;
            mini = mini < sci ? mini : sci;

            int token_len = (int)(mini - text);

            new_token.token_len = token_len;
            text += token_len;
            col += token_len;

            da_push(out, &new_token);
        }
    }
}
