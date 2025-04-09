#pragma once

typedef void col_iter_td;

bool col_iter_inc(col_iter_td *iter);
bool col_iter_dec(col_iter_td *iter);

void *col_iter_get(col_iter_td *iter);

void col_iter_destroy(col_iter_td *iter);
