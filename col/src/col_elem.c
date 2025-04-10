#include "col_elem_priv.h"

#include <string.h>

void col_elem_metadata_init_default(struct col_elem_metadata *to_init, size_t elem_size)
{
    to_init->mv_fn = col_elem_mv_default;
    to_init->cp_fn = col_elem_cp_default;
    to_init->clr_fn = col_elem_clr_default;
    to_init->cmp_fn = NULL;
    to_init->elem_size = elem_size;
}

void col_elem_mv_default(struct col_elem_metadata *md, void *dest, void *src)
{
    memcpy(dest, src, md->elem_size);
}

bool col_elem_cp_default(struct col_elem_metadata *md, void *dest, void *src)
{
    memcpy(dest, src, md->elem_size);
    return true;
}

void col_elem_clr_default(struct col_elem_metadata *md, void *to_clear)
{

}
