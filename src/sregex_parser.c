#include "sregex_parser_priv.h"
#include "sregex_result_priv.h"
#include "sregex_str_priv.h"

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct parse_tree_create_result parse_tree_create(sregex_str_td *borrowed_input_string)
{
    struct parse_tree_create_result result =
    {
        .result_code = SREGEX_RESULT_ALLOC_FAILED,
        .error_index = 0,
        .owned_created = malloc(sizeof(struct parse_tree))
    };
    if(!result.owned_created)
    {
        return result;
    }
    struct sregex_str_iter iter = {
        .borrowed_str_cursor = borrowed_input_string,
        .processed_code_point_count = 0
    };
    result.result_code = parse_expr(&iter, &(result.owned_created->root));
    if(result.result_code)
    {
        free(result.owned_created);
        result.error_index = iter.borrowed_str_cursor - borrowed_input_string;
    }
    return result;
}

sregex_result_td parse_expr(struct sregex_str_iter *rw_cur_pos, struct prod_expr *out)
{
    sregex_result_td result;
    out->choices = NULL;
    out->choices_len = 0;
    
    while(true)
    {
        struct prod_expr_sequence seq_to_add;
        result = parse_sequence(rw_cur_pos, &seq_to_add);
        if(result) goto failed;
        out->choices = reallocarray(out->choices, out->choices_len + 1, sizeof(out->choices[0]));
        out->choices[out->choices_len] = seq_to_add;
        out->choices_len++;
        if(sregex_str_iter_get_char(rw_cur_pos) != sregex_ascii_to_char('|'))
        {
            goto succeeded;
        }
        sregex_str_iter_inc(rw_cur_pos);
    }
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    return result;
failed:
    for(size_t i = 0; i < out->choices_len; i++)
    {
        clear_expr_sequence(&out->choices[i]);
    }
    free(out->choices);
    return result;
}

sregex_result_td parse_sequence(struct sregex_str_iter *rw_cur_pos, struct prod_expr_sequence *out)
{
    sregex_result_td result;
    out->quantified_atoms = NULL;
    out->quantified_atoms_len = 0;

    while(true)
    {
        struct prod_quantified_atom qual_atom_to_add;
        struct sregex_str_iter backtracker = *rw_cur_pos;
        result = parse_quantified_atom(&backtracker, &qual_atom_to_add);
        if(result)
        {
            if(out->quantified_atoms_len == 0)
            {
                *rw_cur_pos = backtracker;
                goto failed;
            }
            else
            {
                // We don't want to indicate a failure position if we actually succeeded.
                goto succeeded;
            }
        }
        else
        {
            *rw_cur_pos = backtracker;
        }
        out->quantified_atoms = reallocarray(out->quantified_atoms, out->quantified_atoms_len + 1, sizeof(out->quantified_atoms[0]));
        out->quantified_atoms[out->quantified_atoms_len] = qual_atom_to_add;
        out->quantified_atoms_len++;
    }
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    return result;
failed:
    for(size_t i = 0; i < out->quantified_atoms_len; i++)
    {
        clear_quantified_atom(&out->quantified_atoms[i]);
    }
    free(out->quantified_atoms);
    return result;
}

sregex_result_td parse_quantified_atom(struct sregex_str_iter *rw_cur_pos, struct prod_quantified_atom *out)
{
    sregex_result_td result;
    struct prod_atom parsed_atom;
    result = parse_atom(rw_cur_pos, &parsed_atom);
    if(result) return result;
    unsigned int qtf_min = 1;
    unsigned int qtf_max = 1;
    struct sregex_str_iter backtracker = *rw_cur_pos;
    result = parse_quantifier(&backtracker, &qtf_min, &qtf_max);
    // Quantifiers are optional, so, whether there was a quantifier or not, we should consider it a success.
    if(!result)
    {
        // a quantifier was parsed, so set the higher order iterator
        *rw_cur_pos = backtracker;
    }
    out->min_incl = qtf_min;
    out->max_incl = qtf_max;
    result = SREGEX_RESULT_SUCCESS;
    return result;
}

sregex_result_td parse_atom(struct sregex_str_iter *rw_cur_pos, struct prod_atom *out)
{
    sregex_result_td result;
    struct sregex_str_iter backtracker = *rw_cur_pos;
    result = parse_grouping(&backtracker, &out->data.grouping);
    if(!result)
    {
        *rw_cur_pos = backtracker;
        out->type = PROD_ATOM_TYPE_GROUPING;
        return result;
    }
    backtracker = *rw_cur_pos;
    result = parse_char_class(&backtracker, &out->data.char_class);
    if(!result)
    {
        *rw_cur_pos = backtracker;
        out->type = PROD_ATOM_TYPE_CHAR_CLASS;
        return result;
    }
    enum prod_char_class_atom_type special_type;
    backtracker = *rw_cur_pos;
    result = parse_char_class_special(&backtracker, &special_type);
    if(!result)
    {
        *rw_cur_pos = backtracker;
        out->type = PROD_ATOM_TYPE_CHAR_CLASS;
        out->data.char_class.atoms = malloc(sizeof(out->data.char_class.atoms[0]));
        out->data.char_class.atoms_len = 1;
        out->data.char_class.atoms[0].type = special_type;
        out->data.char_class.neg = false;
        return result;
    }
    result = parse_char_in_seq(rw_cur_pos, &out->data.char_data);
    return result;
}

sregex_result_td parse_quantifier(struct sregex_str_iter *rw_cur_pos, unsigned int *out_quantifier_min_incl, unsigned int *out_quantifier_max_incl)
{
    sregex_result_td result;
    if(sregex_str_iter_get_char(rw_cur_pos) == sregex_ascii_to_char('?'))
    {
        sregex_str_iter_inc(rw_cur_pos);
        *out_quantifier_min_incl = 0;
        *out_quantifier_max_incl = 1;
        return SREGEX_RESULT_SUCCESS;
    }
    if(sregex_str_iter_get_char(rw_cur_pos) == sregex_ascii_to_char('*'))
    {
        sregex_str_iter_inc(rw_cur_pos);
        *out_quantifier_min_incl = 0;
        *out_quantifier_max_incl = UINT_MAX - 1;
        return SREGEX_RESULT_SUCCESS;
    }
    if(sregex_str_iter_get_char(rw_cur_pos) == sregex_ascii_to_char('+'))
    {
        sregex_str_iter_inc(rw_cur_pos);
        *out_quantifier_min_incl = 1;
        *out_quantifier_max_incl = UINT_MAX - 1;
        return SREGEX_RESULT_SUCCESS;
    }
    // The options for a quantifier are now: {#} {#,#} {,#} {#,}
    if(sregex_str_iter_get_char(rw_cur_pos) != sregex_ascii_to_char('{')) return SREGEX_RESULT_PARSE_FAILED;
    sregex_str_iter_inc(rw_cur_pos);
    unsigned int qty_min = 0;
    unsigned int qty_max = UINT_MAX - 1;
    struct sregex_str_iter backtracker = *rw_cur_pos;
    result = parse_natural_number(&backtracker, &qty_min);
    if(result)
    {
        // This is not necessarily a problem. Could be of the form. {,#}
        if(sregex_str_iter_get_char(rw_cur_pos) != sregex_ascii_to_char(',')) return SREGEX_RESULT_PARSE_FAILED;
        sregex_str_iter_inc(rw_cur_pos);
        result = parse_natural_number(rw_cur_pos, &qty_max);
        if(result) return SREGEX_RESULT_PARSE_FAILED;
    }
    else
    {
        // We've seen {# so we could see } ,#} ,}
        *rw_cur_pos = backtracker;
        if(sregex_str_iter_get_char(rw_cur_pos) == sregex_ascii_to_char(','))
        {
            sregex_str_iter_inc(rw_cur_pos);
            // {#,#} {#,}
            
            result = parse_natural_number(&backtracker, &qty_max);
            // If we failed to parse a NN at this point, dw, we'll check the closing brace after this all
            if(!result) *rw_cur_pos = backtracker;
        }
        else
        {
            // {#}
            qty_max = qty_min;
        }
    }
    // Should be at a point now to close out the bracketed expression
    if(sregex_str_iter_get_char(rw_cur_pos) != sregex_ascii_to_char('}')) return SREGEX_RESULT_PARSE_FAILED;
    *out_quantifier_min_incl = qty_min;
    *out_quantifier_max_incl = qty_max;
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_natural_number(struct sregex_str_iter *rw_cur_pos, unsigned int *out)
{
    sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
    if(c == sregex_ascii_to_char('0'))
    {
        struct sregex_str_iter backtracker = *rw_cur_pos;
        sregex_str_iter_inc(&backtracker);
        c = sregex_str_iter_get_char(&backtracker);
        if(c >= sregex_ascii_to_char('0') && c <= sregex_ascii_to_char('9'))
        {
            *rw_cur_pos = backtracker;
            return SREGEX_RESULT_PARSE_FAILED;
        }
        else
        {
            *out = 0;
            return SREGEX_RESULT_SUCCESS;
        }
    }
    else if(c >= sregex_ascii_to_char('1') && c <= sregex_ascii_to_char('9'))
    {
        *out = 0;
        while(c >= sregex_ascii_to_char('0') && c <= sregex_ascii_to_char('9'))
        {
            *out *= 10;
            *out += (unsigned int)(c - sregex_ascii_to_char('0'));
            sregex_str_iter_inc(rw_cur_pos);
            c = sregex_str_iter_get_char(rw_cur_pos);
        }
        return SREGEX_RESULT_SUCCESS;
    }
    else
    {
        return SREGEX_RESULT_PARSE_FAILED;
    }
}

sregex_result_td parse_char(struct sregex_str_iter *rw_cur_pos, sregex_char_td *out)
{
    sregex_result_td result;
    sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
    switch(c)
    {
        case 0x0000005b: // '['
            // fallthrough
        case 0x0000005d: // ']'
            // fallthrough
        case 0x0000007b: // '{'
            // fallthrough
        case 0x0000007d: // '}'
            // fallthrough
        case 0x00000028: // '('
            // fallthrough
        case 0x00000029: // ')'
            // fallthrough
        case 0x0000007c: // '|'
            // fallthrough
        case 0x0000003f: // '?'
            // fallthrough
        case 0x0000002a: // '*'
            // fallthrough
        case 0x0000002b: // '+'
            // fallthrough
        case 0x0000002e: // '.'
            // fallthrough
        case 0x0000005e: // '^'
            // fallthrough
        case 0x0000002d: // '-'
            return SREGEX_RESULT_PARSE_FAILED;
        case 0x0000005c: // '\\'
            sregex_str_iter_inc(rw_cur_pos);
            c = sregex_str_iter_get_char(rw_cur_pos);
            switch(c)
            {
                case 0x00000061: // 'a' of "\a"
                    *out = 0x00000007;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x00000062: // 'b' of "\b"
                    *out = 0x00000008;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x00000065: // 'e' of "\e"
                    *out = 0x0000001b;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x00000066: // 'f' of "\f"
                    *out = 0x0000000c;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x0000006e: // 'n' of "\n"
                    *out = 0x0000000a;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x00000072: // 'r' of "\r"
                    *out = 0x0000000d;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x00000074: // 't' of "\t"
                    *out = 0x00000009;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x00000076: // 'v' of "\v"
                    *out = 0x0000000b;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                case 0x00000030: // '0'
                    // fallthrough
                case 0x00000031: // '1'
                    // fallthrough
                case 0x00000032: // '2'
                    // fallthrough
                case 0x00000033: // '3'
                    // fallthrough
                case 0x00000034: // '4'
                    // fallthrough
                case 0x00000035: // '5'
                    // fallthrough
                case 0x00000036: // '6'
                    // fallthrough
                case 0x00000037: // '7'
                    // octal
                    if(result = parse_esc_octal(rw_cur_pos, out)) return result;
                    break;
                case 'x': // 'x'
                    // hex XX
                    if(result = parse_esc_hex2(rw_cur_pos, out)) return result;
                    break;
                case 'u': // 'u'
                    // unicode XXXX
                    if(result = parse_esc_hex4(rw_cur_pos, out)) return result;
                    break;
                case 'U': // 'U'
                    // unicode XXXXXXXX
                    if(result = parse_esc_hex8(rw_cur_pos, out)) return result;
                    break;
                case 0x0000005b: // '['
                case 0x0000005d: // ']'
                case 0x0000007b: // '{'
                case 0x0000007d: // '}'
                case 0x00000028: // '('
                case 0x00000029: // ')'
                case 0x0000007c: // '|'
                case 0x0000003f: // '?'
                case 0x0000002a: // '*'
                case 0x0000002b: // '+'
                case 0x0000002e: // '.'
                case 0x0000005e: // '^'
                case 0x0000002d: // '-'
                case 0x0000005c: // '\\'
                    *out = c;
                    sregex_str_iter_inc(rw_cur_pos);
                    break;
                default:
                    return SREGEX_RESULT_PARSE_FAILED;
            }
            break;
        default:
            *out = c;
            sregex_str_iter_inc(rw_cur_pos);
            break;
    }
}

sregex_result_td parse_char_class(struct sregex_str_iter *rw_cur_pos, struct prod_char_class *out)
{
    struct prod_char_class internal_result = 
    {
        .neg = false,
        .atoms = NULL,
        .atoms_len = 0
    };
    sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
    if(c != 0x0000005b) return SREGEX_RESULT_PARSE_FAILED;
    sregex_str_iter_inc(rw_cur_pos);
    c = sregex_str_iter_get_char(rw_cur_pos);
    if(c == 0x0000005e)
    {
        out->neg = true;
        sregex_str_iter_inc(rw_cur_pos);
    }
    struct sregex_str_iter backtracker = *rw_cur_pos;
    struct prod_char_class_atom to_add;
    sregex_result_td result;
    while(true)
    {
        result = parse_char_class_atom(&backtracker, &to_add);
        if(result)
        {
            if(internal_result.atoms_len > 0)
            {
                break;
            }
            else
            {
                *rw_cur_pos = backtracker;
                goto failed;
            }
        }
        *rw_cur_pos = backtracker;
        internal_result.atoms = reallocarray(internal_result.atoms, internal_result.atoms_len + 1, sizeof(internal_result.atoms[0]));
        internal_result.atoms[internal_result.atoms_len] = to_add;
        internal_result.atoms_len++;
    }
    c = sregex_str_iter_get_char(rw_cur_pos);
    if(c != 0x0000005d) goto failed;
    sregex_str_iter_inc(rw_cur_pos);
    *out = internal_result;
    return SREGEX_RESULT_SUCCESS;
failed:
    for(size_t i = 0; i < internal_result.atoms_len; i++) clear_char_class_atom(internal_result.atoms + i);
    free(internal_result.atoms);
    return SREGEX_RESULT_PARSE_FAILED;
}

sregex_result_td parse_char_class_special(struct sregex_str_iter *rw_cur_pos, enum prod_char_class_atom_type *out)
{
    sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
    if(c != 0x0000005c) return SREGEX_RESULT_PARSE_FAILED;
    sregex_str_iter_inc(rw_cur_pos);
    c = sregex_str_iter_get_char(rw_cur_pos);
    switch(c)
    {
        case 0x00000077: // 'w'
            *out = CHAR_CLASS_ATOM_TYPE_ALPHANUMUNDER;
            break;
        case 0x00000057: // 'W'
            *out = CHAR_CLASS_ATOM_TYPE_NALPHANUMUNDER;
            break;
        case 0x00000064: // 'd'
            *out = CHAR_CLASS_ATOM_TYPE_DIGIT;
            break;
        case 0x00000044: // 'D'
            *out = CHAR_CLASS_ATOM_TYPE_NDIGIT;
            break;
        case 0x00000073: // 's'
            *out = CHAR_CLASS_ATOM_TYPE_WHITESPACE;
            break;
        case 0x00000053: // 'S'
            *out = CHAR_CLASS_ATOM_TYPE_NWHITESPACE;
            break;
        default:
            return SREGEX_RESULT_PARSE_FAILED;
    }
    sregex_str_iter_inc(rw_cur_pos);
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_grouping(struct sregex_str_iter *rw_cur_pos, struct prod_expr *out)
{
    sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
    if(c != 0x00000028) return SREGEX_RESULT_PARSE_FAILED; // '('
    sregex_str_iter_inc(rw_cur_pos);
    sregex_result_td result = parse_expr(rw_cur_pos, out);
    if(result) return result;
    c = sregex_str_iter_get_char(rw_cur_pos);
    if(c != 0x00000029) return SREGEX_RESULT_PARSE_FAILED; // ')'
    sregex_str_iter_inc(rw_cur_pos);
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_char_class_atom(struct sregex_str_iter *rw_cur_pos, struct prod_char_class_atom *out)
{
    sregex_result_td result;
    struct sregex_str_iter backtracker = *rw_cur_pos;
    result = parse_char(&backtracker, &(out->data.char_data));
    if(!result)
    {
        *rw_cur_pos = backtracker;
        out->type = CHAR_CLASS_ATOM_TYPE_CHAR;
        return result;
    }
    backtracker = *rw_cur_pos;
    result = parse_char_range(&backtracker, &(out->data.range));
    if(!result)
    {
        *rw_cur_pos = backtracker;
        out->type = CHAR_CLASS_ATOM_TYPE_RANGE;
        return result;
    }
    result = parse_char_class_special(rw_cur_pos, &(out->type));
    return result;
}

sregex_result_td parse_char_range(struct sregex_str_iter *rw_cur_pos, struct prod_char_range *out)
{
    sregex_result_td result;
    sregex_char_td low;
    sregex_char_td high;
    if(result = parse_char(rw_cur_pos, &low)) return result;
    sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
    if(c != 0x0000002d) return SREGEX_RESULT_PARSE_FAILED; // '-'
    sregex_str_iter_inc(rw_cur_pos);
    if(result = parse_char(rw_cur_pos, &high)) return result;
    if(low > high) return SREGEX_RESULT_PARSE_FAILED;
    out->start_incl = low;
    out->end_incl = high;
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_esc_octal(struct sregex_str_iter *rw_cur_pos, sregex_char_td *out)
{
    sregex_char_td accumulator = 0;
    for(int i = 0; i < 3; i++)
    {
        sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
        if(c < sregex_ascii_to_char('0') || c > sregex_ascii_to_char('7')) return SREGEX_RESULT_PARSE_FAILED;
        accumulator <<= 3;
        accumulator += c - sregex_ascii_to_char('0');
        sregex_str_iter_inc(rw_cur_pos);
    }
    // Octal escapes can only represent a single byte.
    // We presuppose it's a UTF-8 stream, so it can really only represent an ASCII value.
    if(accumulator > 0x0000007f) return SREGEX_RESULT_PARSE_FAILED;
    *out = accumulator;
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_esc_hex2(struct sregex_str_iter *rw_cur_pos, sregex_char_td *out)
{
    sregex_char_td accumulator = 0;
    for(int i = 0; i < 2; i++)
    {
        unsigned int nibble;
        sregex_result_td result = parse_hex_digit(rw_cur_pos, &nibble);
        if(result) return result;
        accumulator <<= 4;
        accumulator += (sregex_char_td)nibble;
    }
    // hex2 escape can only represent a single byte.
    // We presuppose it's a UTF-8 stream, so it can really only represent an ASCII value.
    if(accumulator > 0x0000007f) return SREGEX_RESULT_PARSE_FAILED;
    *out = accumulator;
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_esc_hex4(struct sregex_str_iter *rw_cur_pos, sregex_char_td *out)
{
    sregex_char_td accumulator = 0;
    for(int i = 0; i < 4; i++)
    {
        unsigned int nibble;
        sregex_result_td result = parse_hex_digit(rw_cur_pos, &nibble);
        if(result) return result;
        accumulator <<= 4;
        accumulator += (sregex_char_td)nibble;
    }
    *out = accumulator;
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_esc_hex8(struct sregex_str_iter *rw_cur_pos, sregex_char_td *out)
{
    sregex_char_td accumulator = 0;
    for(int i = 0; i < 2; i++)
    {
        unsigned int nibble;
        sregex_result_td result = parse_hex_digit(rw_cur_pos, &nibble);
        if(result) return result;
        accumulator <<= 4;
        accumulator += (sregex_char_td)nibble;
    }
    *out = accumulator;
    return SREGEX_RESULT_SUCCESS;
}
sregex_result_td parse_hex_digit(struct sregex_str_iter *rw_cur_pos, unsigned int *out)
{
    sregex_char_td c = sregex_str_iter_get_char(rw_cur_pos);
    if(c >= 0x00000030 && c <= 0x00000039) *out = c - 0x00000030;
    else if(c >= 0x00000041 && c <= 0x00000046) *out = c - 0x00000037;
    else if(c >= 0x00000061 && c <= 0x00000066) *out = c - 0x00000057;
    else return SREGEX_RESULT_PARSE_FAILED;
    sregex_str_iter_inc(rw_cur_pos);
    return SREGEX_RESULT_SUCCESS;
}

void clear_expr(struct prod_expr *to_clear)
{
    for(size_t i = 0; i < to_clear->choices_len; i++)
    {
        clear_expr_sequence(to_clear->choices + i);
    }
    free(to_clear->choices);
}

void clear_expr_sequence(struct prod_expr_sequence *to_clear)
{
    for(size_t i = 0; i < to_clear->quantified_atoms_len; i++)
    {
        clear_quantified_atom(to_clear->quantified_atoms + i);
    }
    free(to_clear->quantified_atoms);
}

void clear_quantified_atom(struct prod_quantified_atom *to_clear)
{
    clear_atom(&(to_clear->atom));
}

void clear_atom(struct prod_atom *to_clear)
{
    switch(to_clear->type)
    {
        case PROD_ATOM_TYPE_CHAR_CLASS:
            clear_char_class(&(to_clear->data.char_class));
        case PROD_ATOM_TYPE_GROUPING:
            clear_expr(&(to_clear->data.grouping));
    }
}

void clear_char_class(struct prod_char_class *to_clear)
{
    for(size_t i = 0; i < to_clear->atoms_len; i++)
    {
        clear_char_class_atom(to_clear->atoms + i);
    }
    free(to_clear->atoms);
}

void clear_char_class_atom(struct prod_char_class_atom *to_clear)
{
    if(to_clear->type == CHAR_CLASS_ATOM_TYPE_RANGE) clear_char_range(&(to_clear->data.range));
}

void clear_char_range(struct prod_char_range *to_clear)
{

}
