#pragma once

#include <inttypes.h>
#include <stddef.h>

struct dynarray
{
    void (*mv)(uint8_t *source, uint8_t *dest);
    int (*cmp)(uint8_t *a, uint8_t *b);
    uint8_t *data_buf;
    size_t element_size;
    size_t len;
    size_t cap;
};

int dynarray_init_unsorted(struct dynarray *to_init, );