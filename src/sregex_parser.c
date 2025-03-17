#include "sregex_parser_priv.h"
#include "sregex_result_priv.h"

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool peek(char *str, size_t pos, char *tok);
static bool peek_char(char *str, size_t pos, char tok);

sregex_result_t parse_tree_create(char *borrowed_input_string, parse_tree_t **out_created)
{
    sregex_result_t result = SREGEX_RESULT_SUCCESS;
    *out_created = malloc(sizeof(parse_tree_t));
    size_t cursor_pos = 0;
    result = parse_expr(borrowed_input_string, &cursor_pos, *out_created);
    if(result) free(*out_created);
    return result;
}

sregex_result_t parse_expr(char *borrowed_input_string, size_t *rw_cursor_pos, prod_expr_t *out)
{
    sregex_result_t result;
    size_t internal_cursor_pos = *rw_cursor_pos;
    out->choices = NULL;
    out->choices_len = 0;
    
    while(true)
    {
        prod_expr_sequence_t seq_to_add;
        result = parse_sequence(borrowed_input_string, internal_cursor_pos, &seq_to_add);
        if(result) goto failed;
        out->choices = reallocarray(out->choices, out->choices_len + 1, sizeof(out->choices[0]));
        out->choices[out->choices_len] = seq_to_add;
        out->choices_len++;
        if(!peek_char(borrowed_input_string, internal_cursor_pos, '|'))
        {
            goto succeeded;
        }
        internal_cursor_pos++;
    }
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    *rw_cursor_pos = internal_cursor_pos;
    return result;
failed:
    for(size_t i = 0; i < out->choices_len; i++)
    {
        clear_expr_sequence(&out->choices[i]);
    }
    free(out->choices);
    return result;
}

sregex_result_t parse_sequence(char *borrowed_input_string, size_t *rw_cursor_pos, prod_expr_sequence_t *out)
{
    sregex_result_t result;
    size_t internal_cursor_pos = *rw_cursor_pos;
    out->quantified_atoms = NULL;
    out->quantified_atoms_len = 0;

    while(true)
    {
        prod_quantified_atom_t qual_atom_to_add;
        result = parse_quantified_atom(borrowed_input_string, &internal_cursor_pos, &qual_atom_to_add);
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
    *rw_cursor_pos = internal_cursor_pos;
    return result;
failed:
    for(size_t i = 0; i < out->quantified_atoms_len; i++)
    {
        clear_quantified_atom(&out->quantified_atoms[i]);
    }
    free(out->quantified_atoms);
    return result;
}

sregex_result_t parse_quantified_atom(char *borrowed_input_string, size_t *rw_cursor_pos, prod_quantified_atom_t *out)
{
    sregex_result_t result;
    size_t internal_cursor_pos = *rw_cursor_pos;
    prod_atom_t parsed_atom;
    result = parse_atom(borrowed_input_string, &internal_cursor_pos, &parsed_atom);
    if(result) return result;
    unsigned int qtf_min = 1;
    unsigned int qtf_max = 1;
    result = parse_quantifier(borrowed_input_string, &internal_cursor_pos, &qtf_min, &qtf_max);
    // Quantifiers are optional, so, whether there was a quantifier or not, we should consider it a success.
    out->min_incl = qtf_min;
    out->max_incl = qtf_max;
    result = SREGEX_RESULT_SUCCESS;
    *rw_cursor_pos = internal_cursor_pos;
    return result;
}

sregex_result_t parse_atom(char *borrowed_input_string, size_t *rw_cursor_pos, prod_atom_t *out)
{
    sregex_result_t result;
    size_t internal_cursor_pos = *rw_cursor_pos;
    result = parse_grouping(borrowed_input_string, &internal_cursor_pos, &out->data.grouping);
    if(!result)
    {
        out->type = PROD_ATOM_TYPE_GROUPING;
        goto succeeded;
    }
    result = parse_char_class(borrowed_input_string, &internal_cursor_pos, &out->data.char_class);
    if(!result)
    {
        out->type = PROD_ATOM_TYPE_CHAR_CLASS;
        goto succeeded;
    }
    prod_char_class_atom_type_t special_type;
    result = parse_char_class_special(borrowed_input_string, &internal_cursor_pos, &special_type);
    if(!result)
    {
        out->type = special_type;
        goto succeeded;
    }
    result = parse_char_in_seq(borrowed_input_string, &internal_cursor_pos, &out->data.char_data);
    if(result) return result;
succeeded:
    result = SREGEX_RESULT_SUCCESS;
    *rw_cursor_pos = internal_cursor_pos;
    return result;
}

sregex_result_t parse_quantifier(char *borrowed_input_string, size_t *rw_cursor_pos, unsigned int *out_quantifier_min_incl, unsigned int *out_quantifier_max_incl)
{
    sregex_result_t result;
    size_t internal_cursor_pos = *rw_cursor_pos;
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
    internal_cursor_pos++;
    unsigned int qty_min = 0;
    unsigned int qty_max = UINT_MAX - 1;
    result = parse_natural_number(borrowed_input_string, &internal_cursor_pos, &qty_min);
    if(result)
    {
        // This is not necessarily a problem. Could be of the form. {,#}
        if(!peek_char(borrowed_input_string, internal_cursor_pos, ',')) return SREGEX_RESULT_PARSE_FAILED;
        internal_cursor_pos++;
        result = parse_natural_number(borrowed_input_string, &internal_cursor_pos, &qty_max);
        if(result) return SREGEX_RESULT_PARSE_FAILED;
    }
    else
    {
        // We've seen {# so we could see } ,#} ,}
        if(peek_char(borrowed_input_string, &internal_cursor_pos, ','))
        {
            internal_cursor_pos++;
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
    *rw_cursor_pos = internal_cursor_pos;
    return result;
}

sregex_result_t parse_natural_number(char *borrowed_input_string, size_t *rw_cursor_pos, unsigned int *out)
{
    size_t internal_cursor_pos = *rw_cursor_pos;
    unsigned int accumulator = 0;
    for(; borrowed_input_string[internal_cursor_pos] >= '0' && borrowed_input_string[internal_cursor_pos] <= '9'; internal_cursor_pos++)
    {
        accumulator *= 10;
        accumulator += borrowed_input_string[internal_cursor_pos] - '0';
    }
    if(internal_cursor_pos == *rw_cursor_pos) return SREGEX_RESULT_PARSE_FAILED;
    *rw_cursor_pos = internal_cursor_pos;
    *out = accumulator;
    return SREGEX_RESULT_SUCCESS;
}

sregex_result_t parse_char_in_seq(char *borrowed_input_string, size_t *rw_cursor_pos, sregex_char *out)
{

}

sregex_result_t parse_char_class(char *borrowed_input_string, size_t *rw_cursor_pos, prod_atom_t *out);
sregex_result_t parse_char_class_special(char *borrowed_input_string, size_t *rw_cursor_pos, prod_char_class_atom_type_t *out);
sregex_result_t parse_grouping(char *borrowed_input_string, size_t *rw_cursor_pos, prod_expr_t *out);
sregex_result_t parse_char_class_atom(char *borrowed_input_string, size_t *rw_cursor_pos, prod_char_class_atom_t *out);
sregex_result_t parse_char_in_class(char *borrowed_input_string, size_t *rw_cursor_pos, sregex_char *out);
sregex_result_t parse_char_range(char *borrowed_input_string, size_t cursor_pos, prod_char_range_t *out, size_t *out_cursor_pos);

void clear_expr(prod_expr_t *to_clear);
void clear_expr_sequence(prod_expr_sequence_t *to_clear);
void clear_quantified_atom(prod_quantified_atom_t *to_clear);
void clear_atom(prod_atom_t *to_clear);
void clear_char_class(prod_char_class_t *to_clear);
void clear_char_class_atom(prod_char_class_atom_t *to_clear);
void clear_char_range(prod_char_range_t *to_clear);

static bool peek(char *str, size_t pos, char *tok)
{
    if(pos > strlen(str)) return false;
    str += pos;
    pos = 0;
    while(true)
    {
        if(str[pos] != tok[pos]) return false;
        if(str[pos] == '\0') return true;
        pos++;
    }
}

static bool peek_char(char *str, size_t pos, char tok)
{
    if(pos > strlen(str)) return false;
    return str[pos] == tok;
}
