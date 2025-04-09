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
    struct prod_expr root;
};

struct parse_tree_create_result
{
    enum sregex_result result_code;
    size_t             error_index;
};
struct parse_tree_create_result parse_tree_init(sregex_str_td *borrowed_input_string, struct parse_tree *borrowed_to_init);
void parse_tree_clear(struct parse_tree *given_to_clear);
