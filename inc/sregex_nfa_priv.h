#pragma once

#include <stddef.h>

#include "sregex_str_priv.h"

struct sregex_nfa_transition
{
    struct sregex_nfa_state *current_state;
    sregex_char_td current_char;
    struct sregex_nfa_state *next_states;
    size_t next_states_len;
};

struct sregex_nfa_state
{
    struct sregex_nfa_transition *transitions;
    size_t transitions_len;
};

struct sregex_nfa
{
    struct sregex_nfa_state *initial_state;
    struct sregex_nfa_state *current_state;
};
