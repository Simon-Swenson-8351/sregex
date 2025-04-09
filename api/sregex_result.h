#pragma once

enum sregex_result
{
    SREGEX_RESULT_SUCCESS,
    SREGEX_RESULT_PARSE_FAILED,
    SREGEX_RESULT_BAD_ARG,
    SREGEX_RESULT_EOS,
    SREGEX_RESULT_ALLOC_FAILED
};

char const *sregex_result_to_string(enum sregex_result result);
