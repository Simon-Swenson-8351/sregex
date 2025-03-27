#include "sregex_str_priv.h"

#include <stdbool.h>
#include <string.h>

#include "sregex_result_priv.h"

bool sregex_char_validate(sregex_char_td const to_validate)
{
    // Technically you'd need to verify that the code point belongs to an assigned block, but we won't go that deep
    // here.
    return to_validate > 0x0010FFFF;
}

enum sregex_str_validate_state
{
    SREGEX_STR_VALIDATE_STATE_NEW_CHAR,
    SREGEX_STR_VALIDATE_STATE_1_TRAILING_BYTE,
    SREGEX_STR_VALIDATE_STATE_2_TRAILING_BYTES,
    SREGEX_STR_VALIDATE_STATE_3_TRAILING_BYTES_CHECK_OVERLONG,
    SREGEX_STR_VALIDATE_STATE_3_TRAILING_BYTES
};
size_t sregex_str_validate(sregex_str_td *borrowed_to_validate, size_t to_validate_len)
{
    enum sregex_str_validate_state state = SREGEX_STR_VALIDATE_STATE_NEW_CHAR;
    size_t i;
    for(i = 0; i < to_validate_len; i++)
    {
        switch(state)
        {
            case SREGEX_STR_VALIDATE_STATE_NEW_CHAR:
                // We need to avoid overlong encodings
                // 1-byte seq can encode 7 bits
                // 2-byte seq can encode 11 bits
                // Need to check that byte[0], bits[3..5] are non-zero
                // 3-byte seq can encode 16 bits
                // Need to check that byte[0], bits[4..7] are non-zero
                // 4-byte seq can encode 21 bits
                // Need to check that byte[0], bits[5..7] and byte[1], bits[2..3] are non-zero
                if(0x80 & borrowed_to_validate[i] == 0)
                {
                    // assume 1-byte character
                    // State can remain the same, as we'll go to the next character.
                    // Any value for bits[1..7] are valid.
                }
                else if(0xe0 & borrowed_to_validate[i] == 0xc0)
                {
                    // assume 2-byte character
                    // check that byte[0], bits[3..5] are non-zero
                    if(!(0x1c & borrowed_to_validate[i])) return i;
                    state = SREGEX_STR_VALIDATE_STATE_1_TRAILING_BYTE;
                }
                else if(0xf0 & borrowed_to_validate[i] == 0xe0)
                {
                    // assume 3-byte character
                    // check that byte[0], bits[4..7] are non-zero
                    if(!(0x0f & borrowed_to_validate[i])) return i;
                    state = SREGEX_STR_VALIDATE_STATE_2_TRAILING_BYTES;
                }
                else if(0xf8 & borrowed_to_validate[i] == 0xf0)
                {
                    // assume 4-byte character
                    // check that byte[0], bits[5..7] are non-zero
                    if(0x07 & borrowed_to_validate[i])
                    {
                        state = SREGEX_STR_VALIDATE_STATE_3_TRAILING_BYTES;
                    }
                    else
                    {
                        state = SREGEX_STR_VALIDATE_STATE_3_TRAILING_BYTES_CHECK_OVERLONG;
                    }
                }
                else
                {
                    // unexpected sequence
                    return i;
                }
                break;
            case SREGEX_STR_VALIDATE_STATE_1_TRAILING_BYTE:
                if(!(0xc0 & borrowed_to_validate[i] == 0x80)) return i;
                state = SREGEX_STR_VALIDATE_STATE_NEW_CHAR;
                break;
            case SREGEX_STR_VALIDATE_STATE_2_TRAILING_BYTES:
                if(!(0xc0 & borrowed_to_validate[i] == 0x80)) return i;
                state = SREGEX_STR_VALIDATE_STATE_1_TRAILING_BYTE;
                break;
            case SREGEX_STR_VALIDATE_STATE_3_TRAILING_BYTES_CHECK_OVERLONG:
                // byte 2 of a 4-byte character.
                // check that byte[1], bits[2..3] are non-zero if byte[0], bits[5..7] were zero.
                if(!(0x30 & borrowed_to_validate[i])) return i;
                // fallthrough
            case SREGEX_STR_VALIDATE_STATE_3_TRAILING_BYTES:
                if(!(0xc0 & borrowed_to_validate[i] == 0x80)) return i;
                state = SREGEX_STR_VALIDATE_STATE_2_TRAILING_BYTES;
                break;
        }
    }
    if(state != SREGEX_STR_VALIDATE_STATE_NEW_CHAR) return i;
    return SIZE_MAX;
}

sregex_char_td sregex_ascii_to_char(char c)
{
    return *(uint8_t *)(&c);
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
    struct sregex_str_iter it = 
    {
        .borrowed_str_cursor = borrowed,
        .processed_code_point_count = 0
    };
    while(true)
    {
        sregex_char_td decoded = sregex_str_iter_step(&it);
        if(!decoded) break;
    }
    return it.processed_code_point_count;
}

int sregex_char_cmp(sregex_char_td a, sregex_char_td b)
{
    if(a < b) return -1;
    if(a == b) return 0;
    return 1;
}

int sregex_str_cmp(sregex_str_td *borrowed_a, sregex_str_td *borrowed_b)
{
    struct sregex_str_iter iter_a = 
    {
        .borrowed_str_cursor = borrowed_a,
        .processed_code_point_count = 0
    };
    struct sregex_str_iter iter_b = 
    {
        .borrowed_str_cursor = borrowed_b,
        .processed_code_point_count = 0
    };
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

sregex_str_td *sregex_str_cat(sregex_str_td *borrowed_first, sregex_str_td *borrowed_second)
{
    size_t f_len = strlen(borrowed_first);
    size_t s_len = strlen(borrowed_second);
    sregex_str_td *result = malloc(f_len + s_len + 1);
    if(!result) return result;
    memcpy(result, borrowed_first, f_len);
    memcpy(result + f_len, borrowed_second, s_len + 1);
    return result;
}

sregex_char_td sregex_str_iter_step(struct sregex_str_iter *borrowed_iter)
{
    struct sregex_str_to_char_result decoded = sregex_str_to_char(borrowed_iter->borrowed_str_cursor);
    if(decoded.decoded_char != 0)
    {
        borrowed_iter->borrowed_str_cursor += decoded.num_bytes_decoded;
        borrowed_iter->processed_code_point_count++;
    }
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
