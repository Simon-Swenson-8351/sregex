#pragma once

#include <stddef.h>

#include "sregex_str.h"

typedef int dfa_state_id_td;

struct sregex_dfa_state
{
    dfa_state_id_td id;
    struct col_map *sregex_char_td_to_dfa_state_id_td;
};

struct sregex_dfa
{
    dfa_state_id_td start_state_id;
    struct col_sorted_list *dfa_state_id_td_accept_states;
    struct sregex_dfa_state *states;
    size_t states_len;
};

enum sregex_dfa_matcher_mode
{
    SREGEX_DFA_MATCHER_MODE_EXACT, // the string will only match if its entirety matches 
    SREGEX_DFA_MATCHER_MODE_PREFIX, // the string will match if it starts with a matching substring

    SREGEX_DFA_MATCHER_MODE__LEN
};

enum sregex_dfa_matcher_state
{
    SREGEX_DFA_MATCHER_STATE_IN_PROGRESS,
    SREGEX_DFA_MATCHER_STATE_ACCEPTED,
    SREGEX_DFA_MATCHER_STATE_REJECTED,

    SREGEX_DFA_MATCHER_STATE__LEN
};

struct sregex_dfa_matcher
{
    enum sregex_dfa_matcher_mode mode;
    enum sregex_dfa_matcher_state state;
    struct sregex_dfa *borrowed_dfa;
    sregex_str_td *borrowed_input_str;
    struct sregex_str_iter cur_input_str_pos;
    dfa_state_id_td cur_state_id;
};
