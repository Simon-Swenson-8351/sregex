#include "sregex_char_priv.h"

#include <stdbool.h>
#include <string.h>

#include "sregex_result_priv.h"

static void sregex_char_put(char *borrowed_dest, sregex_char_t to_put, size_t *out_bytes_put);

sregex_result_t sregex_char_from_ascii(char c, sregex_char_t *out)
{
    if(!(c & 0x80)) return SREGEX_RESULT_BAD_ARG;
    // We don't want to do anything with the underlying bytes when doing the cast, so we just do a pointer cast instead 
    // of a value cast.
    *out = (sregex_char_t) *((unsigned char *)&c);
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_t sregex_char_from_unicode_point(uint32_t code_point, sregex_char_t *out)
{
    if(code_point > 0x0010FFFF) return SREGEX_RESULT_BAD_ARG;
    *out = code_point;
}

sregex_result_t sregex_char_from_utf8(char *borrowed_utf8_char, sregex_char_t *out, size_t *out_num_bytes)
{
    sregex_char_t temp_out;
    uint8_t *unsigned_utf8 = (uint8_t *)borrowed_utf8_char;
    if(!(unsigned_utf8[0] & 0x80))
    {
        temp_out = (sregex_char_t)unsigned_utf8[0];
        if(out_num_bytes) *out_num_bytes = 1;
    }
    else if(unsigned_utf8[0] & 0xe0 == 0xc0)
    {
        if(unsigned_utf8[1] & 0xc0 != 0x80) return SREGEX_RESULT_BAD_ARG;
        temp_out = ((sregex_char_t)(unsigned_utf8[0] & 0x1f)) << 6;
        temp_out |= (sregex_char_t)(unsigned_utf8[1] & 0x3f);
        if(temp_out < 0x00000080) return SREGEX_RESULT_BAD_ARG;
        if(out_num_bytes) *out_num_bytes = 2;
    }
    else if(unsigned_utf8[0] & 0xf0 == 0xe0)
    {
        if(unsigned_utf8[1] & 0xc0 != 0x80) return SREGEX_RESULT_BAD_ARG;
        if(unsigned_utf8[2] & 0xc0 != 0x80) return SREGEX_RESULT_BAD_ARG;
        temp_out = ((sregex_char_t)(unsigned_utf8[0] & 0x0f)) << 12;
        temp_out |= ((sregex_char_t)(unsigned_utf8[1] & 0x3f)) << 6;
        temp_out |= (sregex_char_t)(unsigned_utf8[2] & 0x3f);
        if(temp_out < 0x00000800) return SREGEX_RESULT_BAD_ARG;
        if(out_num_bytes) *out_num_bytes = 3;
    }
    else if(unsigned_utf8[0] & 0xf8 == 0xf0)
    {
        if(unsigned_utf8[1] & 0xc0 != 0x80) return SREGEX_RESULT_BAD_ARG;
        if(unsigned_utf8[2] & 0xc0 != 0x80) return SREGEX_RESULT_BAD_ARG;
        if(unsigned_utf8[3] & 0xc0 != 0x80) return SREGEX_RESULT_BAD_ARG;
        temp_out = ((sregex_char_t)(unsigned_utf8[0] & 0x07)) << 18;
        temp_out |= ((sregex_char_t)(unsigned_utf8[1] & 0x3f)) << 12;
        temp_out |= ((sregex_char_t)(unsigned_utf8[2] & 0x3f)) << 6;
        temp_out |= (sregex_char_t)(unsigned_utf8[3] & 0x3f);
        if(temp_out < 0x00010000) return SREGEX_RESULT_BAD_ARG;
        if(out_num_bytes) *out_num_bytes = 4;
    }
    else
    {
        return SREGEX_RESULT_BAD_ARG;
    }
    *out = temp_out;
    return SREGEX_RESULT_SUCCESS;
}

int sregex_char_cmp(sregex_char_t a, sregex_char_t b)
{
    if(a < b) return -1;
    if(a == b) return 0;
    return 1;
}

sregex_result_t sregex_str_create_from_ascii(char *given_ascii_str, sregex_str_t **out)
{
    return sregex_str_create_from_utf8(given_ascii_str, out);
}

sregex_result_t sregex_str_create_from_unicode_point_array(uint32_t *borrowed_code_points, size_t code_points_len, sregex_str_t **out)
{
    *out = malloc(sizeof(sregex_str_t));
    (*out)->owned_utf8 = malloc(4 * code_points_len + 1);
    char *cursor = (*out)->owned_utf8;
    for(size_t i = 0; i < code_points_len; i++)
    {
        size_t bytes_put;
        if(borrowed_code_points[i] == 0) break;
        sregex_char_put(cursor, borrowed_code_points[i], &bytes_put);
        cursor += bytes_put;
    }
    *cursor = '\0';
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_t sregex_str_create_from_utf8(char *given_utf8_str, sregex_str_t **out)
{
    *out = malloc(sizeof(sregex_str_t));
    (*out)->owned_utf8 = given_utf8_str;
    return SREGEX_RESULT_SUCCESS;
}

int sregex_str_cmp(sregex_str_t *borrowed_a, sregex_str_t *borrowed_b)
{
    sregex_str_iter_t iter_a;
    sregex_str_iter_init(borrowed_a, &iter_a);
    sregex_str_iter_t iter_b;
    sregex_str_iter_init(borrowed_b, &iter_b);
    while(true)
    {
        sregex_char_t c_a;
        sregex_char_t c_b;
        if(sregex_str_iter_step(&iter_a, &c_a))
        {
            c_a = 0;
        }
        if(sregex_str_iter_step(&iter_b, &c_b))
        {
            c_b = 0;
        }
        if(c_a == 0 && c_b == 0) return 0;
        if(c_a == 0) return -1;
        if(c_b == 0) return 1;
        if(c_a < c_b) return -1;
        if(c_a > c_b) return 1;
        // if the characters are equal, continue until they aren't or we've reached the end of a string.
    }
}

sregex_result_t sregex_str_cat(sregex_str_t *borrowed_first, sregex_str_t *borrowed_second, sregex_str_t **out)
{
    *out = malloc(sizeof(sregex_str_t));
    (*out)->owned_utf8 = malloc(strlen(borrowed_first->owned_utf8) + strlen(borrowed_second->owned_utf8) + 1);
    memcpy((*out)->owned_utf8, borrowed_first->owned_utf8, strlen(borrowed_first->owned_utf8));
    memcpy((*out)->owned_utf8 + strlen(borrowed_first->owned_utf8), borrowed_second->owned_utf8, strlen(borrowed_second->owned_utf8) + 1);
}

void sregex_str_destroy(sregex_str_t *to_destroy)
{
    free(to_destroy->owned_utf8);
    free(to_destroy);
}

sregex_result_t sregex_str_iter_init(sregex_str_t *borrowed_str, sregex_str_iter_t *out)
{
    out->borrowed_str = borrowed_str;
    out->idx = 0;
}

sregex_result_t sregex_str_iter_inc(sregex_str_iter_t *borrowed_iter)
{
    sregex_char_t c;
    return sregex_str_iter_step(borrowed_iter, &c);
}

sregex_result_t sregex_str_iter_step(sregex_str_iter_t *borrowed_iter, sregex_char_t *out)
{
    sregex_char_t c;
    size_t inc;
    sregex_char_from_utf8(borrowed_iter->borrowed_str->owned_utf8[borrowed_iter->idx], &c, &inc);
    if(c == 0) return SREGEX_RESULT_EOS;
    borrowed_iter->idx += inc;
    *out = c;
    return SREGEX_RESULT_SUCCESS;
}

bool sregex_str_iter_has_char(sregex_str_iter_t *borrowed_iter)
{
    sregex_char_t c;
    return !sregex_str_iter_get_char(borrowed_iter->borrowed_str->owned_utf8[borrowed_iter->idx], &c);
}

sregex_result_t sregex_str_iter_get_char(sregex_str_iter_t *borrowed_iter, sregex_char_t *out)
{
    sregex_char_t c;
    sregex_char_from_utf8(borrowed_iter->borrowed_str->owned_utf8[borrowed_iter->idx], &c, NULL);
    if(c == 0) return SREGEX_RESULT_EOS;
    *out = c;
    return SREGEX_RESULT_SUCCESS;
}

static void sregex_char_put(char *borrowed_dest, sregex_char_t to_put, size_t *out_bytes_put)
{
    uint8_t *udest = (uint8_t *)borrowed_dest;
    if(to_put < 0x00000080)
    {
        udest[0] = (uint8_t)to_put;
        if(out_bytes_put) *out_bytes_put = 1;
    }
    else if(to_put < 0x00000800)
    {
        udest[0] = (uint8_t)(0xc0 & (to_put >> 6));
        udest[1] = (uint8_t)(0x80 & (to_put & 0x3f));
        if(out_bytes_put) *out_bytes_put = 2;
    }
    else if(to_put < 0x00010000)
    {
        udest[0] = (uint8_t)(0xe0 & (to_put >> 12));
        udest[1] = (uint8_t)(0x80 & ((to_put >> 6) & 0x3f));
        udest[2] = (uint8_t)(0x80 & (to_put & 0x3f));
        if(out_bytes_put) *out_bytes_put = 3;
    }
    else
    {
        udest[0] = (uint8_t)(0xf0 & (to_put >> 18));
        udest[1] = (uint8_t)(0x80 & ((to_put >> 12) & 0x3f));
        udest[2] = (uint8_t)(0x80 & ((to_put >> 6) & 0x3f));
        udest[3] = (uint8_t)(0x80 & (to_put & 0x3f));
        if(out_bytes_put) *out_bytes_put = 4;
    }
}
