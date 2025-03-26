#include "sregex_parser_priv.h"
#include "sregex_result_priv.h"
#include "sregex_str_priv.h"

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool peek(sregex_str_td *str, size_t pos, sregex_str_td *tok);
static bool peek_char(sregex_str_td *str, size_t pos, sregex_char_td tok);

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
        .borrowed_str_cursor = borrowed_input_string
    };
    result.result_code = parse_expr(&iter, &(result.owned_created->root));
    if(result.result_code) free(result.owned_created);
    return result;
}

sregex_result_td parse_expr(struct sregex_str_iter *rw_cur_pos, struct prod_expr *out)
{
    sregex_result_td result;
    struct sregex_str_iter internal_cursor_pos = *rw_cur_pos;
    out->choices = NULL;
    out->choices_len = 0;
    
    while(true)
    {
        struct prod_expr_sequence seq_to_add;
        result = parse_sequence(&internal_cursor_pos, &seq_to_add);
        if(result) goto failed;
        out->choices = reallocarray(out->choices, out->choices_len + 1, sizeof(out->choices[0]));
        out->choices[out->choices_len] = seq_to_add;
        out->choices_len++;
        if(sregex_str_iter_get_char(&internal_cursor_pos) != sregex_ascii_to_char('|'))
        {
            goto succeeded;
        }
        sregex_str_iter_inc(&internal_cursor_pos);
    }
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    *rw_cur_pos = internal_cursor_pos;
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
    struct sregex_str_iter internal_cursor_pos = *rw_cur_pos;
    out->quantified_atoms = NULL;
    out->quantified_atoms_len = 0;

    while(true)
    {
        struct prod_quantified_atom qual_atom_to_add;
        result = parse_quantified_atom(&internal_cursor_pos, &qual_atom_to_add);
        if(result)
        {
            if(out->quantified_atoms_len == 0) goto failed;
            else goto succeeded;
        }
        out->quantified_atoms = reallocarray(out->quantified_atoms, out->quantified_atoms_len + 1, sizeof(out->quantified_atoms[0]));
        out->quantified_atoms[out->quantified_atoms_len] = qual_atom_to_add;
        out->quantified_atoms_len++;
    }
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    *rw_cur_pos = internal_cursor_pos;
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
    struct sregex_str_iter internal_cursor_pos = *rw_cur_pos;
    struct prod_atom parsed_atom;
    result = parse_atom(&internal_cursor_pos, &parsed_atom);
    if(result) return result;
    unsigned int qtf_min = 1;
    unsigned int qtf_max = 1;
    result = parse_quantifier(&internal_cursor_pos, &qtf_min, &qtf_max);
    // Quantifiers are optional, so, whether there was a quantifier or not, we should consider it a success.
    out->min_incl = qtf_min;
    out->max_incl = qtf_max;
    result = SREGEX_RESULT_SUCCESS;
    *rw_cur_pos = internal_cursor_pos;
    return result;
}

sregex_result_td parse_atom(struct sregex_str_iter *rw_cur_pos, struct prod_atom *out)
{
    sregex_result_td result;
    struct sregex_str_iter internal_cursor_pos = *rw_cur_pos;
    result = parse_grouping(&internal_cursor_pos, &out->data.grouping);
    if(!result)
    {
        out->type = PROD_ATOM_TYPE_GROUPING;
        goto succeeded;
    }
    result = parse_char_class(&internal_cursor_pos, &out->data.char_class);
    if(!result)
    {
        out->type = PROD_ATOM_TYPE_CHAR_CLASS;
        goto succeeded;
    }
    enum prod_char_class_atom_type special_type;
    result = parse_char_class_special(&internal_cursor_pos, &special_type);
    if(!result)
    {
        out->type = special_type;
        goto succeeded;
    }
    result = parse_char_in_seq(&internal_cursor_pos, &out->data.char_data);
    if(result) return result;
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    *rw_cur_pos = internal_cursor_pos;
    return result;
}

sregex_result_td parse_quantifier(struct sregex_str_iter *rw_cur_pos, unsigned int *out_quantifier_min_incl, unsigned int *out_quantifier_max_incl)
{
    sregex_result_td result;
    struct sregex_str_iter internal_cursor_pos = *rw_cur_pos;
    if(peek_char(borrowed_input_string, internal_cursor_pos, '?'))
    {
        *out_quantifier_min_incl = 0;
        *out_quantifier_max_incl = 1;
        internal_cursor_pos++;
        goto succeeded;
    }
    if(peek_char(borrowed_input_string, internal_cursor_pos, '*'))
    {
        *out_quantifier_min_incl = 0;
        *out_quantifier_max_incl = UINT_MAX - 1;
        internal_cursor_pos++;
        goto succeeded;
    }
    if(peek_char(borrowed_input_string, internal_cursor_pos, '+'))
    {
        *out_quantifier_min_incl = 1;
        *out_quantifier_max_incl = UINT_MAX - 1;
        internal_cursor_pos++;
        goto succeeded;
    }
    // The options for a quantifier are now: {#} {#,#} {,#} {#,}
    if(!peek_char(borrowed_input_string, internal_cursor_pos, '{')) return SREGEX_RESULT_PARSE_FAILED;
    sregex_str_iter_inc(&internal_cursor_pos);
    unsigned int qty_min = 0;
    unsigned int qty_max = UINT_MAX - 1;
    result = parse_natural_number(&internal_cursor_pos, &qty_min);
    if(result)
    {
        // This is not necessarily a problem. Could be of the form. {,#}
        if(!peek_char(borrowed_input_string, internal_cursor_pos, ',')) return SREGEX_RESULT_PARSE_FAILED;
        sregex_str_iter_inc(&internal_cursor_pos);
        result = parse_natural_number(&internal_cursor_pos, &qty_max);
        if(result) return SREGEX_RESULT_PARSE_FAILED;
    }
    else
    {
        // We've seen {# so we could see } ,#} ,}
        if(peek_char(borrowed_input_string, &internal_cursor_pos, ','))
        {
            sregex_str_iter_inc(&internal_cursor_pos);
            // {#,#} {#,}
            result = parse_natural_number(borrowed_input_string, &internal_cursor_pos, &qty_max);
            // If we failed to parse a NN at this point, dw, we'll check the closing brace after this all
        }
        else
        {
            // {#}
            qty_max = qty_min;
        }
    }
    // Should be at a point now to close out the bracketed expression
    if(!peek_char(borrowed_input_string, internal_cursor_pos, '}')) return SREGEX_RESULT_PARSE_FAILED;
    *out_quantifier_min_incl = qty_min;
    *out_quantifier_max_incl = qty_max;
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    *rw_cur_pos = internal_cursor_pos;
    return result;
}

sregex_result_td parse_natural_number(struct sregex_str_iter *rw_cur_pos, unsigned int *out)
{
    struct sregex_str_iter internal_cursor_pos = *rw_cur_pos;
    if(borrowed_input_string[*rw_cursor_pos] < '0' || borrowed_input_string[*rw_cursor_pos] > '9') return SREGEX_RESULT_PARSE_FAILED;
    if(borrowed_input_string[*rw_cursor_pos] == '0' && // started with 0
        *rw_cursor_pos + 1 < strlen(borrowed_input_string) && // and next position will not be off the end of the string
        borrowed_input_string[*rw_cursor_pos + 1] >= '0' && // and next character is another digit
        borrowed_input_string[*rw_cursor_pos + 1] <= '9')
    {
        return SREGEX_RESULT_PARSE_FAILED;
    }
    unsigned int accumulator = 0;
    for(; borrowed_input_string[internal_cursor_pos] >= '0' && borrowed_input_string[internal_cursor_pos] <= '9'; internal_cursor_pos++)
    {
        accumulator *= 10;
        accumulator += borrowed_input_string[internal_cursor_pos] - '0';
    }
    if(internal_cursor_pos == *rw_cursor_pos) return SREGEX_RESULT_PARSE_FAILED;
    *rw_cur_pos = internal_cursor_pos;
    *out = accumulator;
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_td parse_char_in_seq(struct sregex_str_iter *rw_cur_pos, sregex_char_td *out)
{
    size_t internal_cursor_pos = *rw_cur_pos;
    switch(borrowed_input_string[*rw_cursor_pos])
    {
    case '[':
        // intentional fallthrough
    case ']':
        // intentional fallthrough
    case '{':
        // intentional fallthrough
    case '}':
        // intentional fallthrough
    case '(':
        // intentional fallthrough
    case ')':
        // intentional fallthrough
    case '|':
        // intentional fallthrough
    case '?':
        // intentional fallthrough
    case '*':
        // intentional fallthrough
    case '+':
        // intentional fallthrough
    case '.':
        return SREGEX_RESULT_PARSE_FAILED;
    case '\\':
        internal_cursor_pos++;
        if(internal_cursor_pos >= strlen(borrowed_input_string)) return SREGEX_RESULT_PARSE_FAILED;
        int escRep = -1;
        switch(borrowed_input_string[internal_cursor_pos])
        {
        case 'a':
            *out = '\x07';
            break;
        case 'b':
            *out = '\x08';
            break;
        case 'e':
            *out = '\x1b';
            break;
        case 'f':
            *out = '\x0c';
            break;
        case 'n':
            *out = '\x0a';
            break;
        case 'r':
            *out = '\x0d';
            break;
        case 't':
            *out = '\x09';
            break;
        case 'v':
            *out = '\x0b';
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            // octal
            escRep = (int)(borrowed_input_string[internal_cursor_pos] - '0');
            internal_cursor_pos++;
            if(borrowed_input_string[internal_cursor_pos] <= '0' || borrowed_input_string[internal_cursor_pos] >= '9') return SREGEX_RESULT_PARSE_FAILED;
            escRep <<= 3;
            escRep += (int)(borrowed_input_string[internal_cursor_pos] - '0');
            internal_cursor_pos++;
            if(borrowed_input_string[internal_cursor_pos] <= '0' || borrowed_input_string[internal_cursor_pos] >= '9') return SREGEX_RESULT_PARSE_FAILED;
            escRep <<= 3;
            escRep += (int)(borrowed_input_string[internal_cursor_pos] - '0');
            internal_cursor_pos++;
            break;
        case 'x':
            // hex XX
            escRep = 0;
            switch()
            break;
        case 'u':
            // unicode XXXX
            break;
        case 'U':
            // unicode XXXXXXXX
            break;
        case '[':
        case ']':
        case '{':
        case '}':
        case '(':
        case ')':
        case '|':
        case '?':
        case '*':
        case '+':
        case '.':
        case '/':
        case '\\':
            *out = borrowed_input_string[internal_cursor_pos];
            internal_cursor_pos++;
            break;
        default:
            return SREGEX_RESULT_PARSE_FAILED;
            break;
        }
        break;
    default:
        *out = borrowed_input_string[*rw_cursor_pos];
        internal_cursor_pos++;
        break;
    }
}

sregex_result_td parse_char_class(char *borrowed_input_string, struct sregex_str_iter *rw_cursor_pos, struct prod_atom *out);
sregex_result_td parse_char_class_special(char *borrowed_input_string, struct sregex_str_iter *rw_cursor_pos, enum prod_char_class_atom_type *out);
sregex_result_td parse_grouping(char *borrowed_input_string, struct sregex_str_iter *rw_cursor_pos, struct prod_expr *out);
sregex_result_td parse_char_class_atom(char *borrowed_input_string, struct sregex_str_iter *rw_cursor_pos, struct prod_char_class_atom *out);
sregex_result_td parse_char_in_class(char *borrowed_input_string, struct sregex_str_iter *rw_cursor_pos, sregex_char_td *out);
sregex_result_td parse_char_range(char *borrowed_input_string, struct sregex_str_iter *rw_cursor_pos, struct prod_char_range *out);

void clear_expr(struct prod_expr *to_clear);
void clear_expr_sequence(struct prod_expr_sequence *to_clear);
void clear_quantified_atom(struct prod_quantified_atom *to_clear);
void clear_atom(struct prod_atom *to_clear);
void clear_char_class(struct prod_char_class *to_clear);
void clear_char_class_atom(struct prod_char_class_atom *to_clear);
void clear_char_range(struct prod_char_range *to_clear);

static bool peek(sregex_str_td *str, size_t pos, sregex_str_td *tok)
{
    if(pos > strlen(str)) return false;
    str += pos;
    pos = 0;
    while(true)
    {
        if(tok[pos] == '\0')
        if(str[pos] != tok[pos]) return false;
        if(str[pos] == '\0') return true;
        pos++;
    }
}

static bool peek_char(sregex_str_td *str, size_t pos, sregex_char_td tok)
{
    if(pos > strlen(str)) return false;
    return str[pos] == tok;
}
