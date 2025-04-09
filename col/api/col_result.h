#pragma once

#include <inttypes.h>

enum col_result
{
    COL_RESULT_SUCCESS,
    COL_RESULT_ALLOC_FAILED,
    COL_RESULT_BAD_ARG,

    COL_RESULT__LEN
};

char const *col_result_to_string(enum col_result result);
