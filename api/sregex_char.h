#pragma once

#include "sregex_result.h";

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint32_t sregex_char_t; // code-point
typedef struct sregex_str sregex_str_t;
typedef struct sregex_str_iter
{
    sregex_str_t *borrowed_str;
    size_t        idx;
} sregex_str_iter_t;

sregex_result_t sregex_char_from_ascii(char c, sregex_char_t *out);
sregex_result_t sregex_char_from_unicode_point(uint32_t code_point, sregex_char_t *out);
sregex_result_t sregex_char_from_utf8(char *borrowed_utf8_char, sregex_char_t *out, size_t *out_num_bytes);

int sregex_char_cmp(sregex_char_t a, sregex_char_t b);



sregex_result_t sregex_str_create_from_ascii(char *given_ascii_str, sregex_str_t **out);
sregex_result_t sregex_str_create_from_unicode_point_array(uint32_t *borrowed_code_points, size_t code_points_len, sregex_str_t **out);
sregex_result_t sregex_str_create_from_utf8(char *given_utf8_str, sregex_str_t **out);

int sregex_str_cmp(sregex_str_t *borrowed_a, sregex_str_t *borrowed_b);
sregex_result_t sregex_str_cat(sregex_str_t *borrowed_first, sregex_str_t *borrowed_second, sregex_str_t **out);

void sregex_str_destroy(sregex_str_t *to_destroy);



sregex_result_t sregex_str_iter_init(sregex_str_t *borrowed_str, sregex_str_iter_t *out);

sregex_result_t sregex_str_iter_inc(sregex_str_iter_t *borrowed_iter);
sregex_result_t sregex_str_iter_step(sregex_str_iter_t *borrowed_iter, sregex_char_t *out);

bool sregex_str_iter_has_char(sregex_str_iter_t *borrowed_iter);
sregex_result_t sregex_str_iter_get_char(sregex_str_iter_t *borrowed_iter, sregex_char_t *out);
