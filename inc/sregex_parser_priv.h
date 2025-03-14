#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sregex_result_priv.h"
#include "sregex_char_priv.h"

typedef enum prod_char_class_atom_type
{
    CHAR_CLASS_ATOM_TYPE_CHAR,
    CHAR_CLASS_ATOM_TYPE_RANGE,
    CHAR_CLASS_ATOM_TYPE_ALPHANUMUNDER,
    CHAR_CLASS_ATOM_TYPE_NALPHANUMUNDER,
    CHAR_CLASS_ATOM_TYPE_DIGIT,
    CHAR_CLASS_ATOM_TYPE_NDIGIT,
    CHAR_CLASS_ATOM_TYPE_WHITESPACE,
    CHAR_CLASS_ATOM_TYPE_NWHITESPACE,
    CHAR_CLASS_ATOM_TYPE__LEN
} prod_char_class_atom_type_t;

typedef struct prod_char_range
{
    sregex_char start_incl;
    sregex_char end_incl;
} prod_char_range_t;

typedef struct prod_char_class_atom
{
    prod_char_class_atom_type_t type;
    union
    {
        sregex_char       char_data;
        prod_char_range_t range;
    } data;
} prod_char_class_atom_t;

typedef struct prod_char_class
{
    bool neg;
    prod_char_class_atom_t *atoms;
    size_t atoms_len;
} prod_char_class_t;

typedef enum prod_atom_type
{
    PROD_ATOM_TYPE_CHAR,
    PROD_ATOM_TYPE_CHAR_CLASS,
    PROD_ATOM_TYPE_GROUPING,
    PROD_ATOM_TYPE__LEN
} prod_atom_type_t;

typedef struct prod_atom
{
    prod_atom_type_t type;
    union 
    {
        sregex_char        char_data;
        prod_char_class_t  char_class;
        prod_expr_t        grouping;
    } data;
    
} prod_atom_t;

typedef struct prod_quantified_atom
{
    prod_atom_t atom;
    size_t min_incl;
    size_t max_incl;
} prod_quantified_atom_t;

typedef struct prod_expr_sequence
{
    prod_quantified_atom_t *quantified_atoms;
    size_t quantified_atoms_len;
} prod_expr_sequence_t;

typedef struct prod_expr
{
    prod_expr_sequence_t *choices;
    size_t choices_len;
} prod_expr_t;

sregex_result_t parse_expr(char *borrowed_input_string, size_t cursor_pos, prod_expr_t *out, size_t *out_cursor_pos);
sregex_result_t parse_sequence(char *borrowed_input_string, size_t cursor_pos, prod_expr_sequence_t *out, size_t *out_cursor_pos);
sregex_result_t parse_quantified_atom(char *borrowed_input_string, size_t cursor_pos, prod_quantified_atom_t *out, size_t *out_cursor_pos);
sregex_result_t parse_atom(char *borrowed_input_string, size_t cursor_pos, prod_atom_t *out, size_t *out_cursor_pos);
sregex_result_t parse_quantifier(char *borrowed_input_string, size_t cursor_pos, size_t *out_quantifier_min_incl, size_t *out_quantifier_max_incl, size_t *out_cursor_pos);
sregex_result_t parse_natural_number(char *borrowed_input_string, size_t cursor_pos, unsigned int *out, size_t *out_cursor_pos);
sregex_result_t parse_char_in_seq(char *borrowed_input_string, size_t cursor_pos, sregex_char *out, size_t *out_cursor_pos);
sregex_result_t parse_char_class(char *borrowed_input_string, size_t cursor_pos, prod_atom_t *out, size_t *out_cursor_pos);
sregex_result_t parse_char_class_special(char *borrowed_input_string, size_t cursor_pos, prod_char_class_atom_type_t *out, size_t *out_cursor_pos);
sregex_result_t parse_grouping(char *borrowed_input_string, size_t cursor_pos, prod_expr_t *out, size_t *out_cursor_pos);
sregex_result_t parse_char_class_atom(char *borrowed_input_string, size_t cursor_pos, prod_char_class_atom_t *out, size_t *out_cursor_pos);
sregex_result_t parse_char_in_class(char *borrowed_input_string, size_t cursor_pos, sregex_char *out, size_t *out_cursor_pos);
sregex_result_t parse_char_range(char *borrowed_input_string, size_t cursor_pos, prod_char_range_t *out, size_t *out_cursor_pos);

void clear_expr_sequence(prod_expr_sequence_t *to_destroy);
void clear_quantified_atom(prod_quantified_atom_t *to_destroy);
void clear_atom(prod_atom_t *to_destroy);
void clear_char_class(prod_char_class_t *to_destroy);
void clear_char_class_atom(prod_char_class_atom_t *to_destroy);
