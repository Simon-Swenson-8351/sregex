#pragma once

#include <stdbool.h>
#include <stddef.h>

struct col_elem_metadata;

typedef void (*col_elem_mv_fn)  (struct col_elem_metadata *md, void *dest, void *src);
typedef bool (*col_elem_cp_fn)  (struct col_elem_metadata *md, void *dest, void *src);
typedef void (*col_elem_clr_fn) (struct col_elem_metadata *md, void *to_clear);
typedef bool (*col_elem_eq_fn)  (struct col_elem_metadata *md, void *a, void *b);
typedef int  (*col_elem_cmp_fn) (struct col_elem_metadata *md, void *a, void *b);

struct col_elem_metadata
{
    col_elem_mv_fn   mv_fn;     // required
    col_elem_cp_fn   cp_fn;     // required
    col_elem_clr_fn  clr_fn;    // required
    col_elem_eq_fn   eq_fn;     // optional, needed for searching
    col_elem_cmp_fn  cmp_fn;    // optional, needed for sorted data structures
    size_t           elem_size; // required, must be > 0
};

void
col_elem_metadata_init_default(
    struct col_elem_metadata *to_init,
    size_t elem_size
);
// memcpy
void
col_elem_mv_default(
    struct col_elem_metadata *md,
    void *dest,
    void *src
);
// memcpy
bool
col_elem_cp_default(
    struct col_elem_metadata *md,
    void *dest,
    void *src
);
// no-op
void
col_elem_clr_default(
    struct col_elem_metadata *md,
    void *to_clear
);
