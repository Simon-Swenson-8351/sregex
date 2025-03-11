#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sregex_result.h"

typedef struct sregex sregex_t;

sregex_result_t sregex_create(char *borrowed_regex, sregex_t **out);
sregex_result_t sregex_destroy(sregex_t *given_to_destroy);

sregex_result_t sregex_match_exact(sregex_t *borrowed_regex, char *borrowed_string, bool *out);
sregex_result_t sregex_match_at(sregex_t *borrowed_regex, char *borrowed_string, size_t idx, size_t *out_match_len);

typedef struct sregex_match_once_result
{
    size_t start_incl;
    size_t match_len;
} sregex_match_once_result_t;
sregex_result_t sregex_match_once(sregex_t *borrowed_regex, char *borrowed_string, sregex_match_once_result_t *out);

typedef struct sregex_match_all_result
{
    // The caller must free owned_matches if it is set by sregex_match_all
    sregex_match_once_result_t *owned_matches;
    size_t matches_len;
} sregex_match_all_result_t;
sregex_result_t sregex_match_all(sregex_t *borrowed_regex, char *string, sregex_match_all_result_t *out);
