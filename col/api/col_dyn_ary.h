#pragma once

#include <stddef.h>

#include "col_allocator.h"
#include "col_elem.h"
#include "col_iter.h"
#include "col_result.h"

struct col_dyn_ary
{
    struct col_allocator allocator;
    struct col_elem_metadata elem_metadata;
    uint8_t *data;
    size_t len;
    size_t cap;
    float growth_factor;
};

extern float const COL_DYN_ARY_DEFAULT_GROWTH_FACTOR;

// init, move, copy, clear
enum col_result
col_dyn_ary_init(
    struct col_dyn_ary *to_init,
    struct col_allocator *allocator,
    struct col_elem_metadata *elem_metadata,
    size_t initial_cap,
    float growth_factor
);
enum col_result
col_dyn_ary_copy(
    struct col_dyn_ary *dest, // must be uninitialized or cleared
    struct col_dyn_ary *src
);
void
col_dyn_ary_clear(
    struct col_dyn_ary *to_clear
);

// metadata
size_t
col_dyn_ary_len(
    struct col_dyn_ary *self
);

// element-level operations
// insertion via move
enum col_result
col_dyn_ary_insert_at(
    struct col_dyn_ary *dyn_ary,
    void *to_insert,
    size_t index
);
enum col_result
col_dyn_ary_push_back(
    struct col_dyn_ary *dyn_ary,
    void *to_insert
);

// retrieval via borrowing
void *
col_dyn_ary_get(
    struct col_dyn_ary const *dyn_ary,
    size_t idx
);
// removal via move
enum col_result
col_dyn_ary_rm(
    struct col_dyn_ary *dyn_ary,
    size_t idx,
    void *removed_elem // must be uninitialized or cleared
);
// searching
int
col_dyn_ary_lin_search(
    struct col_dyn_ary *dyn_ary,
    void *elem
);
int
col_dyn_ary_bin_search(
    struct col_dyn_ary *dyn_ary,
    void *elem
);


// list-level operations
// removes unused data in the dynamic array such that capacity == length
enum col_result
col_dyn_ary_trim(
    struct col_dyn_ary *dyn_ary
);
enum col_result
col_dyn_ary_cat(
    struct col_dyn_ary *first,
    struct col_dyn_ary *second
);

void
col_dyn_ary_sort(
    struct col_dyn_ary *to_sort
);
