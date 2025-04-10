#pragma once

#include <stddef.h>

#include "col_elem_fndef.h"

typedef void col_sorted_list_td;

col_sorted_list_td *col_sorted_list_create_avl_tree(col_elem_move move_fn, col_elem_copy copy_fn, col_elem_cmp cmp_fn, size_t initial_size, size_t element_size);
