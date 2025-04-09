#pragma once

#include <stdbool.h>

typedef void (*col_elem_move)(void *dest, void *src);
typedef bool (*col_elem_copy)(void *dest, void *src);
typedef void (*col_elem_clear)(void *to_clear);
typedef int (*col_elem_cmp)(void *a, void *b);
