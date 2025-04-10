#include "col_dyn_ary_priv.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "col_result.h"

float const COL_DYN_ARY_DEFAULT_GROWTH_FACTOR = 2.0;

static enum col_result expand(struct col_dyn_ary *dyn_ary);

enum col_result col_dyn_ary_init(struct col_dyn_ary *to_init, struct col_allocator *allocator, struct col_elem_metadata *elem_metadata, size_t initial_cap, float growth_factor)
{
    if(!to_init) return COL_RESULT_BAD_ARG;
    if(!allocator) return COL_RESULT_BAD_ARG;
    if(!elem_metadata) return COL_RESULT_BAD_ARG;
    to_init->allocator = *allocator;
    to_init->elem_metadata = *elem_metadata;
    if(initial_cap > 0)
    {
        to_init->data = allocator->malloc(allocator, elem_metadata->elem_size * initial_cap);
        if(!to_init->data) return COL_RESULT_ALLOC_FAILED;
    }
    else
    {
        to_init->data = NULL;
    }
    to_init->len = 0;
    to_init->cap = initial_cap;
    to_init->growth_factor = growth_factor;
    return COL_RESULT_SUCCESS;
}

enum col_result col_dyn_ary_copy(struct col_dyn_ary *dest, struct col_dyn_ary *src)
{
    if(!dest) return COL_RESULT_BAD_ARG;
    if(!src) return COL_RESULT_BAD_ARG;
    dest->allocator = src->allocator;
    dest->elem_metadata = src->elem_metadata;
    dest->len = src->len;
    dest->cap = src->cap;
    dest->growth_factor = src->growth_factor;
    if(src->cap > 0)
    {
        dest->data = malloc(src->elem_metadata.elem_size * src->cap);
        if(!dest->data) return COL_RESULT_ALLOC_FAILED;
    }
    else
    {
        dest->data = NULL;
    }
    for(size_t i = 0; i < src->len; i++)
    {
        if(!src->elem_metadata.cp_fn(&src->elem_metadata, dest->data + src->elem_metadata.elem_size * i, src->data + src->elem_metadata.elem_size * i))
        {
            // clean up the previously copied elements
            for(size_t j = 0; j < i; j++) src->elem_metadata.clr_fn(&src->elem_metadata, dest->data + src->elem_metadata.elem_size * j);
            free(dest->data);
            return COL_RESULT_COPY_ELEM_FAILED;
        }
    }
    return COL_RESULT_SUCCESS;
}

void col_dyn_ary_clear(struct col_dyn_ary *to_clear)
{
    if(to_clear->data)
    {
        for(size_t i = 0; i < to_clear->len; i++)
        {
            to_clear->elem_metadata.clr_fn(&to_clear->elem_metadata, to_clear->data + to_clear->elem_metadata.elem_size * i);
        }
        free(to_clear->data);
    }
}

enum col_result col_dyn_ary_insert_at(struct col_dyn_ary *dyn_ary, void *to_insert, size_t index)
{
    enum col_result result;
    if(index > dyn_ary->len) return COL_RESULT_IDX_OOB;
    if(dyn_ary->len == dyn_ary->cap)
    {
        if(result = expand(dyn_ary)) return result;
    }
    for(size_t i = dyn_ary->len; i > index; i--)
    {
        dyn_ary->elem_metadata.mv_fn(
            &dyn_ary->elem_metadata,
            dyn_ary->data + dyn_ary->elem_metadata.elem_size * i,
            dyn_ary->data + dyn_ary->elem_metadata.elem_size * (i - 1)
        );
    }
    dyn_ary->elem_metadata.mv_fn(
        &dyn_ary->elem_metadata,
        dyn_ary->data + dyn_ary->elem_metadata.elem_size * index,
        to_insert
    );
    dyn_ary->len++;
    return COL_RESULT_SUCCESS;
}

enum col_result col_dyn_ary_push_back(struct col_dyn_ary *dyn_ary, void *to_insert)
{
    return col_dyn_ary_insert_at(dyn_ary, to_insert, dyn_ary->len);
}

void *col_dyn_ary_get(struct col_dyn_ary const *dyn_ary, size_t idx)
{
    if(idx >= dyn_ary->len) return NULL;
    return dyn_ary->data + dyn_ary->elem_metadata.elem_size * idx;
}

enum col_result col_dyn_ary_rm(struct col_dyn_ary *dyn_ary, size_t idx, void *removed_elem)
{
    if(idx >= dyn_ary->len) return COL_RESULT_IDX_OOB;
    dyn_ary->elem_metadata.mv_fn(
        &dyn_ary->elem_metadata,
        removed_elem,
        dyn_ary->data + dyn_ary->elem_metadata.elem_size * idx
    );
    for(size_t i = idx; i + 1 < dyn_ary->len; i++)
    {
        dyn_ary->elem_metadata.mv_fn(
            &dyn_ary->elem_metadata,
            dyn_ary->data + dyn_ary->elem_metadata.elem_size * i,
            dyn_ary->data + dyn_ary->elem_metadata.elem_size * (i + 1)
        );
    }
    dyn_ary->len--;
    return COL_RESULT_SUCCESS;
}

enum col_result col_dyn_ary_cat(struct col_dyn_ary *first, struct col_dyn_ary *second);

void col_dyn_ary_sort(struct col_dyn_ary *to_sort, col_elem_cmp cmp_fn);

static void move_elem(void *dest, void *src, size_t elem_size, col_elem_move move_fn)
{
    if(move_fn)
    {
        move_fn(dest, src);
    }
    else
    {
        memcpy(dest, src, elem_size);
    }
}

static bool copy_elem(void *dest, void *src, size_t elem_size, col_elem_copy copy_fn)
{
    if(copy_fn)
    {
        return copy_fn(dest, src);
    }
    else
    {
        memcpy(dest, src, elem_size);
        return true;
    }
}

static void clear_elem(void *to_clear, col_elem_clear clear_fn)
{
    if(clear_fn)
    {
        clear_fn(to_clear);
    }
}

static enum col_result expand(struct col_dyn_ary *dyn_ary)
{
    size_t new_cap = (size_t)((float)(dyn_ary->cap) * dyn_ary->growth_factor);
    if(new_cap == dyn_ary->cap) new_cap++;
    uint8_t *new_buf = dyn_ary->allocator.malloc(&dyn_ary->allocator, dyn_ary->elem_metadata.elem_size * new_cap);
    if(!new_buf) return COL_RESULT_ALLOC_FAILED;
    for(size_t i = 0; i < dyn_ary->len; i++)
    {
        dyn_ary->elem_metadata.mv_fn(
            &dyn_ary->elem_metadata,
            new_buf + dyn_ary->elem_metadata.elem_size * i,
            dyn_ary->data + dyn_ary->elem_metadata.elem_size * i
        );
    }
    dyn_ary->allocator.free(&dyn_ary->allocator, dyn_ary->data);
    dyn_ary->data = new_buf;
    dyn_ary->cap = new_cap;
    return COL_RESULT_SUCCESS;
}