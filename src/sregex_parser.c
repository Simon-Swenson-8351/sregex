#include "sregex_parser_priv.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool peek(char *str, size_t pos, char *tok);

sregex_result_t parse_expr(char *borrowed_input_string, size_t cursor_pos, prod_expr_t *out, size_t *out_cursor_pos)
{
    sregex_result_t result = 0;
    out->choices = NULL;
    out->choices_len = 0;
    
    while(true)
    {
        prod_expr_sequence_t seq_to_add;
        result = parse_sequence(borrowed_input_string, cursor_pos, &seq_to_add, &cursor_pos);
        if(result) goto cleanup_out;
        out->choices = reallocarray(out->choices, out->choices_len + 1, sizeof(out->choices[0]));
        out->choices[out->choices_len] = seq_to_add;
        out->choices_len++;
        if(!peek(borrowed_input_string, cursor_pos, "|")) goto end;
        cursor_pos++;
    }
cleanup_out:
    for(size_t i = 0; i < out->choices_len; i++)
    {
        clear_expr_sequence(&out->choices[i]);
    }
    free(out->choices);
end:
    return result;
}

sregex_result_t parse_sequence(char *borrowed_input_string, size_t cursor_pos, prod_expr_sequence_t *out, size_t *out_cursor_pos)
{
    sregex_result_t result = 0;
    out->quantified_atoms = NULL;
    out->quantified_atoms_len = 0;

    while(true)
    {
        prod_quantified_atom_t qual_atom_to_add;
        result = parse_quantified_atom(borrowed_input_string, cursor_pos, &qual_atom_to_add, &cursor_pos);
        if(result)
        {
            if(out->quantified_atoms_len == 0)
            {
                goto cleanup_out;
            }
            else
            {
                // we had at least one match, which is what we were expecting.
                result = 0;
                goto end;
            }
        }
        out->quantified_atoms = reallocarray(out->quantified_atoms, out->quantified_atoms_len + 1, sizeof(out->quantified_atoms[0]));
        out->quantified_atoms[out->quantified_atoms_len] = qual_atom_to_add;
        out->quantified_atoms_len++;
    }
cleanup_out:
    for(size_t i = 0; i < out->quantified_atoms_len; i++)
    {
        clear_quantified_atom(&out->quantified_atoms[i]);
    }
    free(out->quantified_atoms);
end:
    return result;
}

sregex_result_t parse_quantified_atom(char *borrowed_input_string, size_t cursor_pos, prod_quantified_atom_t *out, size_t *out_cursor_pos)
{
    sregex_result_t result = 0;
    prod_atom_t parsed_atom;
    result = parse_atom(borrowed_input_string, cursor_pos, &parsed_atom, &cursor_pos);
    if(result) goto cleanup_out;
    out->atom = parsed_atom;
    result = parse_quantifier(borrowed_input_string, cursor_pos, &parsed_atom->);

cleanup_out:
    clear_quantified_atom(out);
end:
    return result;
}

sregex_result_t parse_atom(char *borrowed_input_string, size_t cursor_pos, prod_atom_t *out, size_t *out_cursor_pos);
sregex_result_t parse_quantifier(char *borrowed_input_string, size_t cursor_pos, size_t *quantifier_min_incl, size_t *quantifier_max_incl, size_t *out_cursor_pos);
sregex_result_t parse_natural_number(char *borrowed_input_string, size_t cursor_pos, unsigned int *out, size_t *out_cursor_pos);
sregex_result_t parse_char_in_seq(char *borrowed_input_string, size_t cursor_pos, sregex_char *out, size_t *out_cursor_pos);
sregex_result_t parse_char_class(char *borrowed_input_string, size_t cursor_pos, prod_atom_t *out, size_t *out_cursor_pos);
sregex_result_t parse_char_class_special(char *borrowed_input_string, size_t cursor_pos, prod_char_class_atom_type_t *out, size_t *out_cursor_pos);
sregex_result_t parse_grouping(char *borrowed_input_string, size_t cursor_pos, prod_expr_t *out, size_t *out_cursor_pos);
sregex_result_t parse_char_class_atom(char *borrowed_input_string, size_t cursor_pos, prod_char_class_atom_t *out, size_t *out_cursor_pos);
sregex_result_t parse_char_in_class(char *borrowed_input_string, size_t cursor_pos, sregex_char *out, size_t *out_cursor_pos);
sregex_result_t parse_char_range(char *borrowed_input_string, size_t cursor_pos, prod_char_range_t *out, size_t *out_cursor_pos);

static bool peek(char *str, size_t pos, char *tok)
{
    if(pos > strlen(str)) return false;
    str = str + pos;
    pos = 0;
    while(true)
    {
        if(str[pos] != tok[pos]) return false;
        if(str[pos] == '\0') return true;
    }
}
