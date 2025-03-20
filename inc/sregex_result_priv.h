#pragma once

#include "sregex_result.h"

enum sregex_result_priv
{
    SREGEX_RESULT_SUCCESS,
    SREGEX_RESULT_PARSE_FAILED,
    SREGEX_RESULT_BAD_ARG,
    SREGEX_RESULT_EOS,
    SREGEX_RESULT_ALLOC_FAILED
};
