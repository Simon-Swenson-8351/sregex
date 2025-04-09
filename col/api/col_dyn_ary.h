#pragma once

#include <stddef.h>

#include "col_elem_fndef.h"
#include "col_iter.h"
#include "col_result.h"

struct col_dyn_ary
{
    col_elem_move move_fn;
    col_elem_clear clear_fn;
    uint8_t *data;
    size_t elem_size;
    size_t len;
    size_t cap;
};

// If move_fn is NULL, we'll just use memcpy to move.
enum col_result col_dyn_ary_init(struct col_dyn_ary *to_init, col_elem_move move_fn, col_elem_clear clear_fn, size_t initial_cap, size_t element_size);

enum col_result col_dyn_ary_copy(struct col_dyn_ary *dest, struct col_dyn_ary *src, col_elem_copy elem_copy_fn);

void col_dyn_ary_clear(struct col_dyn_ary *to_clear);

enum col_result col_dyn_ary_insert_at(struct col_dyn_ary *dyn_ary, void *to_insert, size_t index);
enum col_result col_dyn_ary_push_back(struct col_dyn_ary *dyn_ary, void *to_insert);

void *col_dyn_ary_get(struct col_dyn_ary const *dyn_ary, size_t idx);

enum col_result col_dyn_ary_rm(struct col_dyn_ary *dyn_ary, void *removed_elem);

enum col_result col_dyn_ary_cat(struct col_dyn_ary *first, struct col_dyn_ary *second);

void col_dyn_ary_sort(struct col_dyn_ary *to_sort, col_elem_cmp cmp_fn);
