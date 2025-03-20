#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "sregex_result_priv.h"
#include "sregex_str_priv.h"

enum prod_char_class_atom_type
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
};

struct prod_char_range
{
    sregex_char_td start_incl;
    sregex_char_td end_incl;
};

struct prod_char_class_atom
{
    enum prod_char_class_atom_type type;
    union
    {
        sregex_char_td char_data;
        struct prod_char_range range;
    } data;
};

struct prod_char_class
{
    bool neg;
    struct prod_char_class_atom *atoms;
    size_t atoms_len;
};

enum prod_atom_type
{
    PROD_ATOM_TYPE_CHAR,
    PROD_ATOM_TYPE_CHAR_CLASS,
    PROD_ATOM_TYPE_GROUPING,
    PROD_ATOM_TYPE__LEN
};

struct prod_atom
{
    enum prod_atom_type type;
    union 
    {
        sregex_char_td        char_data;
        struct prod_char_class  char_class;
        struct prod_expr        grouping;
    } data;
};

struct prod_quantified_atom
{
    struct prod_atom atom;
    unsigned int min_incl;
    unsigned int max_incl;
};

struct prod_expr_sequence
{
    struct prod_quantified_atom *quantified_atoms;
    size_t quantified_atoms_len;
};

struct prod_expr
{
    struct prod_expr_sequence *choices;
    size_t choices_len;
};

struct parse_tree
{
    struct prod_expr root; // amusing since it's borrowing from itself, an element in owned_node_pool
};

sregex_result_td parse_tree_create(sregex_str_td *borrowed_input_string, struct parse_tree **out_created);
sregex_result_td parse_tree_destroy(struct parse_tree *given_to_destroy);

sregex_result_td parse_expr
(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_expr *out
);
sregex_result_td parse_sequence
(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_expr_sequence *out
);
sregex_result_td parse_quantified_atom(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_quantified_atom *out
);
sregex_result_td parse_atom(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_atom *out
);
sregex_result_td parse_quantifier(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    unsigned int *out_quantifier_min_incl,
    unsigned int *out_quantifier_max_incl
);
sregex_result_td parse_natural_number(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    unsigned int *out
);
sregex_result_td parse_char_in_seq(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    sregex_char_td *out
);
sregex_result_td parse_char_class(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_atom *out
);
sregex_result_td parse_char_class_special(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    enum prod_char_class_atom_type *out
);
sregex_result_td parse_grouping(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_expr *out
);
sregex_result_td parse_char_class_atom(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_char_class_atom *out
);
sregex_result_td parse_char_in_class(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    sregex_char_td *out
);
sregex_result_td parse_char_range(
    sregex_str_td *borrowed_input_string,
    size_t *rw_cursor_pos,
    struct prod_char_range *out
);

void clear_expr           (struct prod_expr            *borrowed_to_clear);
void clear_expr_sequence  (struct prod_expr_sequence   *borrowed_to_clear);
void clear_quantified_atom(struct prod_quantified_atom *borrowed_to_clear);
void clear_atom           (struct prod_atom            *borrowed_to_clear);
void clear_char_class     (struct prod_char_class      *borrowed_to_clear);
void clear_char_class_atom(struct prod_char_class_atom *borrowed_to_clear);
void clear_char_range     (struct prod_char_range      *borrowed_to_clear);
