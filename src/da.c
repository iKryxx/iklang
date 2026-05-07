#include "da.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void da_grow(da_t *arr) {
    if (arr->is_static) return;

    size_t old_cap = arr->cap;

    size_t new_cap = old_cap == 0 ? 10 : old_cap * 2;
    void *new_data = realloc(arr->data, new_cap * arr->stride);
    if (!new_data) {
        perror("realloc");
        exit(1);
    }
    arr->data = new_data;
    arr->cap = new_cap;
}

void da_init(da_t *arr, size_t init_cap, size_t stride) {
    arr->data = malloc(init_cap * stride);
    arr->length = 0;
    arr->cap = init_cap;
    arr->stride = stride;
    arr->is_static = false;
    arr->compare_cb = NULL;
}

void *da_push(da_t *arr, const void *data) {
    if (arr->is_static) return NULL;

    if (arr->length == arr->cap)
        da_grow(arr);

    unsigned char *dst = (unsigned char *)arr->data + arr->stride * arr->length++;

    memcpy(dst, data, arr->stride);
    return dst;
}

void *da_pop(da_t *arr) {
    if (arr->is_static) return NULL;

    if (arr->length == 0)
        return NULL;
    return (unsigned char *)arr->data + arr->stride * --arr->length;
}

void *da_get(const da_t *arr, size_t i) {
    if (i >= arr->length)
        return NULL;
    return (unsigned char *)arr->data + arr->stride * i;
}

void da_remove(da_t *arr, size_t i) {
    if (i >= arr->length)
        return;
    if (i == arr->length - 1) {
        arr->length--;
        return;
    }
    memmove((unsigned char *)arr->data + arr->stride * i,
            (unsigned char *)arr->data + arr->stride * (i + 1),
            (--arr->length - i) * arr->stride);
}

void *da_insert(da_t *arr, size_t i, const void *data) {
    if (arr->is_static || i >= arr->length) return NULL;

    if (arr->length == arr->cap)
        da_grow(arr);

    unsigned char *dst = (unsigned char *)arr->data + arr->stride * i;

    memmove(dst + arr->stride, dst, arr->stride * (arr->length - i));
    memcpy(dst, data, arr->stride);
    arr->length++;
    return dst;
}

void da_free(da_t *arr) {
    if (arr->is_static) return;
    free(arr->data);
    arr->data = NULL;
    arr->stride = 0;
    arr->length = 0;
    arr->cap = 0;
}

bool da_has(const da_t *arr, void *data) {
    if (arr->compare_cb == NULL) return false;

    for (size_t i = 0; i < arr->length; i++) {
        if (arr->compare_cb(arr, i, data) == 0) return true;
    }

    return false;
}

size_t da_idx_of(const da_t *arr, void *data) {
    if(arr->compare_cb == NULL) return (size_t)-1;

    for(size_t i = 0; i < arr->length; i++) {
        if (arr->compare_cb(arr, i, data) == 0) return i;
    }

    return (size_t)-1;
}
