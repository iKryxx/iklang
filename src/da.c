#include "da.h"
#include <string.h>

static void da_grow(da_t *arr) {
    size_t old_cap = arr->cap;

    size_t new_cap = old_cap == 0 ? 10 : old_cap * 2;
    arr->data = realloc(arr->data, new_cap * arr->stride);
    arr->cap = new_cap;
}

void da_init(da_t *arr, size_t init_cap, size_t stride) {
    arr->data = malloc(init_cap * stride);
    arr->length = 0;
    arr->cap = init_cap;
    arr->stride = stride;
}

void da_push(da_t *arr, void *data) {
    if (arr->length == arr->cap)
        da_grow(arr);

    memcpy((unsigned char *)arr->data + arr->stride * arr->length++, data,
           arr->stride);
}

void *da_pop(da_t *arr) {
    if (arr->length == 0)
        return NULL;
    return (unsigned char *)arr->data + arr->stride * --arr->length;
}

void *da_get(const da_t *arr, size_t i) {
    if (i >= arr->length)
        return NULL;
    return (unsigned char *)arr->data + arr->stride * i;
}

void da_free(da_t *arr) {
    free(arr->data);
    arr->data = NULL;
    arr->stride = 0;
    arr->length = 0;
    arr->cap = 0;
}
