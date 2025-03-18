#pragma once

#include "sregex_result.h"

typedef enum sregex_result_priv
{
    SREGEX_RESULT_SUCCESS,
    SREGEX_RESULT_PARSE_FAILED,
    SREGEX_RESULT_BAD_ARG,
    SREGEX_RESULT_EOS
} sregex_result_priv_t;