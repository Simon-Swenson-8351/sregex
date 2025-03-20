#include "sregex_str_priv.h"

#include <stdbool.h>
#include <string.h>

#include "sregex_result_priv.h"

static bool sregex_utf8_char_validate(sregex_str_td *to_validate, size_t to_validate_len, size_t *out_num_bytes);

bool sregex_char_validate(sregex_char_td to_validate)
{
    return to_validate > 0x0010FFFF;
}

size_t sregex_str_validate(sregex_str_td *borrowed_to_validate, size_t to_validate_len)
{
    size_t decoded_size;
    for(size_t i = 0; i < to_validate_len; i += decoded_size)
    {
        if(!sregex_utf8_char_validate(borrowed_to_validate + i, to_validate_len - i, &decoded_size)) return i;
    }
    return SIZE_MAX;
}

struct sregex_str_to_char_result sregex_str_to_char(sregex_str_td *borrowed_str)
{
    struct sregex_str_to_char_result result;
    if(!(borrowed_str[0] & 0x80))
    {
        result.decoded_char = borrowed_str[0];
        result.num_bytes_decoded = 1;
    }
    else if(borrowed_str[0] & 0xe0 == 0xc0)
    {
        result.decoded_char = ((sregex_char_td)(borrowed_str[0] & 0x1f)) << 6;
        result.decoded_char |= (sregex_char_td)(borrowed_str[1] & 0x3f);
        result.num_bytes_decoded = 2;
    }
    else if(borrowed_str[0] & 0xf0 == 0xe0)
    {
        result.decoded_char = ((sregex_char_td)(borrowed_str[0] & 0x0f)) << 12;
        result.decoded_char |= ((sregex_char_td)(borrowed_str[1] & 0x3f)) << 6;
        result.decoded_char |= (sregex_char_td)(borrowed_str[2] & 0x3f);
        result.num_bytes_decoded = 3;
    }
    else
    {
        result.decoded_char = ((sregex_char_td)(borrowed_str[0] & 0x07)) << 18;
        result.decoded_char |= ((sregex_char_td)(borrowed_str[1] & 0x3f)) << 12;
        result.decoded_char |= ((sregex_char_td)(borrowed_str[2] & 0x3f)) << 6;
        result.decoded_char |= (sregex_char_td)(borrowed_str[3] & 0x3f);
        result.num_bytes_decoded = 4;
    }
    return result;
}

struct sregex_char_to_str_result sregex_char_to_str(sregex_char_td to_put)
{
    struct sregex_char_to_str_result result;
    if(to_put < 0x00000080)
    {
        result.str[0] = (uint8_t)to_put;
        result.str[1] = 0x00;
        result.num_bytes_encoded = 1;
    }
    else if(to_put < 0x00000800)
    {
        result.str[0] = (uint8_t)(0xc0 & (to_put >> 6));
        result.str[1] = (uint8_t)(0x80 & (to_put & 0x3f));
        result.str[2] = 0x00;
        result.num_bytes_encoded = 2;
    }
    else if(to_put < 0x00010000)
    {
        result.str[0] = (uint8_t)(0xe0 & (to_put >> 12));
        result.str[1] = (uint8_t)(0x80 & ((to_put >> 6) & 0x3f));
        result.str[2] = (uint8_t)(0x80 & (to_put & 0x3f));
        result.str[3] = 0x00;
        result.num_bytes_encoded = 3;
    }
    else
    {
        result.str[0] = (uint8_t)(0xf0 & (to_put >> 18));
        result.str[1] = (uint8_t)(0x80 & ((to_put >> 12) & 0x3f));
        result.str[2] = (uint8_t)(0x80 & ((to_put >> 6) & 0x3f));
        result.str[3] = (uint8_t)(0x80 & (to_put & 0x3f));
        result.str[4] = 0x00;
        result.num_bytes_encoded = 4;
    }
    return result;
}

size_t sregex_str_char_count(sregex_str_td *borrowed)
{
    size_t result = 0;
    struct sregex_str_iter it = { .borrowed_str_cursor = borrowed };
    while(true)
    {
        sregex_char_td decoded = sregex_str_iter_step(&it);
        if(!decoded) break;
        result++;
    }
    return result;
}

int sregex_char_cmp(sregex_char_td a, sregex_char_td b)
{
    if(a < b) return -1;
    if(a == b) return 0;
    return 1;
}

int sregex_str_cmp(sregex_str_td *borrowed_a, sregex_str_td *borrowed_b)
{
    struct sregex_str_iter iter_a = { .borrowed_str_cursor = borrowed_a };
    struct sregex_str_iter iter_b = { .borrowed_str_cursor = borrowed_b };
    while(true)
    {
        sregex_char_td c_a = sregex_str_iter_step(&iter_a);
        sregex_char_td c_b = sregex_str_iter_step(&iter_b);
        if(c_a == 0 && c_b == 0) return 0;
        if(c_a < c_b) return -1;
        if(c_a > c_b) return 1;
        // if the characters are equal and non-zero, continue until they aren't or we've reached the end of a string.
    }
}

bool sregex_str_cat(sregex_str_td *borrowed_first, sregex_str_td *borrowed_second, sregex_str_td **out)
{
    size_t f_len = strlen(borrowed_first);
    size_t s_len = strlen(borrowed_second);
    *out = malloc(f_len + s_len + 1);
    if(!(*out)) return false;
    memcpy(*out, borrowed_first, f_len);
    memcpy(*out + f_len, borrowed_second, s_len + 1);
    return true;
}

sregex_char_td sregex_str_iter_step(struct sregex_str_iter *borrowed_iter)
{
    struct sregex_str_to_char_result decoded = sregex_str_to_char(borrowed_iter->borrowed_str_cursor);
    if(decoded.decoded_char != 0) borrowed_iter->borrowed_str_cursor += decoded.num_bytes_decoded;
    return decoded.decoded_char;
}

bool sregex_str_iter_inc(struct sregex_str_iter *borrowed_iter)
{
    sregex_char_td decoded = sregex_str_iter_step(borrowed_iter);
    return decoded;
}

sregex_char_td sregex_str_iter_get_char(struct sregex_str_iter *borrowed_iter)
{
    return sregex_str_to_char(borrowed_iter->borrowed_str_cursor).decoded_char;
}

bool sregex_str_iter_has_char(struct sregex_str_iter *borrowed_iter)
{
    return sregex_str_iter_get_char(borrowed_iter);
}

static bool sregex_utf8_char_validate(sregex_str_td *to_validate, size_t to_validate_len, size_t *out_num_bytes)
{
    // see sregex_str_to_char, as the code is similar
    if(to_validate_len == 0) return true; // an empty string is vacuously valid
    sregex_char_td code_point;
    if(!(to_validate[0] & 0x80))
    {
        code_point = (sregex_char_td)to_validate[0];
        if(out_num_bytes) *out_num_bytes = 1;
    }
    else if(to_validate[0] & 0xe0 == 0xc0)
    {
        if(to_validate_len < 2) return false;
        if(to_validate[1] & 0xc0 != 0x80) return false;
        code_point = ((sregex_char_td)(to_validate[0] & 0x1f)) << 6;
        code_point |= (sregex_char_td)(to_validate[1] & 0x3f);
        if(code_point < 0x00000080) return false;
        if(out_num_bytes) *out_num_bytes = 2;
    }
    else if(to_validate[0] & 0xf0 == 0xe0)
    {
        if(to_validate_len < 3) return false;
        if(to_validate[1] & 0xc0 != 0x80) return false;
        if(to_validate[2] & 0xc0 != 0x80) return false;
        code_point = ((sregex_char_td)(to_validate[0] & 0x0f)) << 12;
        code_point |= ((sregex_char_td)(to_validate[1] & 0x3f)) << 6;
        code_point |= (sregex_char_td)(to_validate[2] & 0x3f);
        if(code_point < 0x00000800) return false;
        if(out_num_bytes) *out_num_bytes = 3;
    }
    else if(to_validate[0] & 0xf8 == 0xf0)
    {
        if(to_validate_len < 4) return false;
        if(to_validate[1] & 0xc0 != 0x80) return false;
        if(to_validate[2] & 0xc0 != 0x80) return false;
        if(to_validate[3] & 0xc0 != 0x80) return false;
        code_point = ((sregex_char_td)(to_validate[0] & 0x07)) << 18;
        code_point |= ((sregex_char_td)(to_validate[1] & 0x3f)) << 12;
        code_point |= ((sregex_char_td)(to_validate[2] & 0x3f)) << 6;
        code_point |= (sregex_char_td)(to_validate[3] & 0x3f);
        if(code_point < 0x00010000) return false;
        if(out_num_bytes) *out_num_bytes = 4;
    }
    else
    {
        return false;
    }
    return true;
}
