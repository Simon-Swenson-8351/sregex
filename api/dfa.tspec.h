#pragma once

#include <stddef.h>

#include "dfa_common.h"

typedef struct MACROSPEC_DFA_STATE MACROSPEC_DFA_STATE;

#define COLN_DATA_TYPENAME MACROSPEC_DFA_STATE
#define COLN_DATA_COPY MACROSPEC_DFA_STATE_copy
#define COLN_DATA_CLEAR MACROSPEC_DFA_STATE_clear
#define COLN_HEADER

#include "array_list.t.h"

#undef COLN_HEADER
#undef COLN_DATA_CLEAR
#undef COLN_DATA_COPY
#undef COLN_DATA_TYPENAME

typedef struct MACROSPEC_DFA
{
    MACROSPEC_ALLOCATOR *allocator;
    MACROSPEC_DFA_STATE_LIST states;
} MACROSPEC_DFA;

sregex_result MACROSPEC_DFA_init(
    MACROSPEC_DFA *to_init,
    MACROSPEC_ALLOCATOR *allocator);
sregex_result MACROSPEC_DFA_clear(
    MACROSPEC_DFA *to_clear);
sregex_result MACROSPEC_DFA_create_state(
    MACROSPEC_DFA *dfa,
    size_t *out_new_state_id);
sregex_result MACROSPEC_DFA_set_accept_state(
    MACROSPEC_DFA *dfa,
    size_t state_id);
sregex_result MACROSPEC_DFA_add_transition(
    MACROSPEC_DFA *dfa,
    size_t from_state_id,
    size_t to_state_id,
    MACROSPEC_CHARACTER *character);
sregex_result MACROSPEC_DFA_add_transitions(
    MACROSPEC_DFA *dfa,
    size_t from_state_id,
    size_t to_state_id,
    MACROSPEC_CHARACTER *characters,
    size_t characters_len);
sregex_result MACROSPEC_DFA_find_matches(
    MACROSPEC_DFA *dfa,
    MACROSPEC_CHARACTER_COLLECTION_ITERATOR *str_iter,
    dfa_match_type match_type,
    MACROSPEC_CHARACTER_PAIR_LIST *out_matches);

bool MACROSPEC_DFA_STATE_copy(MACROSPEC_DFA_STATE *dest, MACROSPEC_DFA_STATE *src);
void MACROSPEC_DFA_STATE_clear(MACROSPEC_DFA_STATE *to_clear);
