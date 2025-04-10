#include "col_allocator_priv.h"

#include <stdlib.h>

static void *std_malloc(struct col_allocator *priv, size_t size);
static void std_free(struct col_allocator *priv, void *to_free);

struct col_allocator col_allocator_std = 
{
    .malloc = std_malloc,
    .free = std_free,
    .priv   = NULL
};

struct col_allocator *col_allocator_get_std(void)
{
    return &col_allocator_std;
}

static void *std_malloc(struct col_allocator *priv, size_t size)
{
    return malloc(size);
}

static void std_free(struct col_allocator *priv, void *to_free)
{
    free(to_free);
}
