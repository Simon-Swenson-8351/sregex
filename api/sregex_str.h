#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "sregex_result.h";

typedef uint32_t sregex_char_td; // code-point
typedef uint8_t sregex_str_td; // utf-8
// view into utf-8 string
// These can be initialized outside of library code by setting the borrowed_str_cursor pointer to a utf-8 string.
struct sregex_str_iter
{
    sregex_str_td *borrowed_str_cursor;
};

// All of the functions which take sregex_char_td and sregex_str_td expect well-formed code points and utf-8 strings 
// respectively. The following validation functions are provided for convenience for the user to ensure that the code 
// points and utf-8 strings are valid.
bool sregex_char_validate(sregex_char_td to_validate);
// returns the index of first error, or SIZE_MAX if there were no errors
size_t sregex_str_validate(sregex_str_td *borrowed_to_validate, size_t to_validate_len);

// These are code point <-> utf-8 conversion functions
struct sregex_str_to_char_result
{
    sregex_char_td decoded_char;
    size_t         num_bytes_decoded;
};
struct sregex_str_to_char_result sregex_str_to_char(sregex_str_td *borrowed_str);
struct sregex_char_to_str_result
{
    sregex_str_td str[5];
    size_t        num_bytes_encoded;
};
struct sregex_char_to_str_result sregex_char_to_str(sregex_char_td to_put);

size_t sregex_str_char_count(sregex_str_td *borrowed);

// code point comparison
int sregex_char_cmp(sregex_char_td a, sregex_char_td b);
// lexicographical comparison by code point
int sregex_str_cmp(sregex_str_td *borrowed_a, sregex_str_td *borrowed_b);
// out is allocated using malloc. result is false only if allocation failed.
bool sregex_str_cat(sregex_str_td *borrowed_first, sregex_str_td *borrowed_second, sregex_str_td **out);

sregex_char_td sregex_str_iter_step(struct sregex_str_iter *borrowed_iter);
bool sregex_str_iter_inc(struct sregex_str_iter *borrowed_iter);
sregex_char_td sregex_str_iter_get_char(struct sregex_str_iter *borrowed_iter);
bool sregex_str_iter_has_char(struct sregex_str_iter *borrowed_iter);
