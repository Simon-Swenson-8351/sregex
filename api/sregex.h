#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sregex_result.h"
#include "sregex_str.h"

struct sregex;
struct sregex_match
{
    size_t start_incl;
    size_t match_size;
};

sregex_result_td sregex_create(sregex_str_td *borrowed_regex_str, struct sregex **out);
sregex_result_td sregex_destroy(struct sregex *given_to_destroy);

sregex_result_td sregex_match_exact(struct sregex *borrowed_regex, char *borrowed_string, bool *out);
sregex_result_td sregex_match_at(struct sregex *borrowed_regex, char *borrowed_string, size_t idx, size_t *out_match_len);

struct sregex_match_once_result
{
    sregex_result_td    result_type;
    struct sregex_match match;
};
struct sregex_match_once_result sregex_match_once(struct sregex *borrowed_regex, char *borrowed_string);

struct sregex_match_all_result
{
    sregex_result_td     result_type;
    struct sregex_match *owned_matches;
    size_t               matches_len;
};
struct sregex_match_all_result sregex_match_all(struct sregex *borrowed_regex, char *string);
