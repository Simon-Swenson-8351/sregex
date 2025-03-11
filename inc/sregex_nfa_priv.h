#pragma once

#include <stddef.h>

#include "sregex_char_priv.h"

typedef sregex_nfa_state sregex_nfa_state_t;

typedef struct sregex_nfa_transition
{
    sregex_nfa_state_t *current_state;
    sregex_char current_char;
    sregex_nfa_state_t *next_states;
    size_t next_states_len;
} sregex_nfa_transition_t;

typedef struct sregex_nfa_state
{
    sregex_nfa_transition_t *transitions;
    size_t transitions_len;
} sregex_nfa_state_t;

typedef struct sregex_nfa
{
    sregex_nfa_state_t *initial_state;
    sregex_nfa_state_t *current_state;
} sregex_nfa_t;
