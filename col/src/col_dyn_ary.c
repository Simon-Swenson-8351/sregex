#include "col_dyn_ary_priv.h"

#include <stdlib.h>

#include "col_result.h"

enum col_result col_dyn_ary_init(struct col_dyn_ary *to_init, col_elem_move move_fn, col_elem_clear clear_fn, size_t initial_cap, size_t element_size)
{
    if(!to_init) return COL_RESULT_BAD_ARG;
    if(element_size == 0) return COL_RESULT_BAD_ARG;
    to_init->move_fn = move_fn;
    to_init->clear_fn = clear_fn;
    if(initial_cap > 0)
    {
        to_init->data = malloc(element_size * initial_cap);
        if(!to_init->data) return COL_RESULT_ALLOC_FAILED;
    }
    else
    {
        to_init->data = NULL;
    }
    to_init->elem_size = element_size;
    to_init->len = 0;
    to_init->cap = initial_cap;
}

enum col_result col_dyn_ary_copy(struct col_dyn_ary *dest, struct col_dyn_ary *src, col_elem_copy elem_copy_fn)
{
    if(!dest) return COL_RESULT_BAD_ARG;
    if(!src) return COL_RESULT_BAD_ARG;
    if(!elem_copy_fn) return COL_RESULT_BAD_ARG;
    dest->move_fn = src->move_fn;
    dest->elem_size = src->elem_size;
    dest->len = src->len;
    dest->cap = src->cap;
    if(dest->cap > 0)
    {
        dest->data = malloc(dest->elem_size * dest->cap);
        if(!dest->data) return COL_RESULT_ALLOC_FAILED;
    }
    else
    {
        dest->data = NULL;
    }
    for(size_t i = 0; i < src->len; i++)
    {
        if(!elem_copy_fn(dest->data + dest->elem_size * i, src->data + src->elem_size * i))
        {
            if(i > 0)
            {
                for(size_t j = i - 1; j != SIZE_MAX; j--)
                {
                    
                }
            }
        }
    }
}

void col_dyn_ary_clear(struct col_dyn_ary *to_clear)
{
    if(to_clear->data) free(to_clear->data);
}

enum col_result col_dyn_ary_insert_at(struct col_dyn_ary *dyn_ary, void *to_insert, size_t index);
enum col_result col_dyn_ary_push_back(struct col_dyn_ary *dyn_ary, void *to_insert);

void *col_dyn_ary_get(struct col_dyn_ary const *dyn_ary, size_t idx);

enum col_result col_dyn_ary_rm(struct col_dyn_ary *dyn_ary, void *removed_elem);

enum col_result col_dyn_ary_cat(struct col_dyn_ary *first, struct col_dyn_ary *second);

void col_dyn_ary_sort(struct col_dyn_ary *to_sort, col_elem_cmp cmp_fn);
