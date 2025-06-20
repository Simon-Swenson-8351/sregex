#include "dfa.tspec.h"

#define COLN_FIRST_TYPENAME MACROSPEC_CHARACTER
#define COLN_FIRST_COPY MACROSPEC_CHARACTER_copy
#define COLN_FIRST_CLEAR MACROSPEC_CHARACTER_clear
#define COLN_SECOND_TYPENAME size_t
#define COLN_HEADER
#define COLN_IMPL

#include "pair.t.h"

#undef COLN_IMPL
#undef COLN_HEADER
#undef COLN_SECOND_TYPENAME
#undef COLN_FIRST_CLEAR
#undef COLN_FIRST_COPY
#undef COLN_FIRST_TYPENAME

#define COLN_DATA_TYPENAME MACROSPEC_CHARACTER_MACROSPEC_DFA_STATE_pair
#define COLN_DATA_COPY MACROSPEC_CHARACTER_MACROSPEC_DFA_STATE_pair_copy
#define COLN_DATA_CLEAR MACROSPEC_CHARACTER_MACROSPEC_DFA_STATE_pair_clear
#define COLN_DATA_HASH(pair) (MACROSPEC_CHARACTER_hash(&(pair->first)))
#define COLN_HEADER
#define COLN_IMPL

#include "hash_table.t.h"
// typename: MACROSPEC_CHARACTER_MACROSPEC_DFA_STATE_pair_hash_table

#undef COLN_IMPL
#undef COLN_HEADER
#undef COLN_DATA_HASH
#undef COLN_DATA_CLEAR
#undef COLN_DATA_COPY
#undef COLN_DATA_TYPENAME


struct MACROSPEC_DFA_STATE
{
    MACROSPEC_CHARACTER_STATE_MAP transitions;
    bool accept;
};

sregex_result MACROSPEC_UNICODEPOINT_DFA_init(
    MACROSPEC_UNICODEPOINT_DFA *to_init,
    MACROSPEC_UNICODEPOINT_DFA_ALLOC *allocator)
{
    assert(to_init);
    assert(allocator);
    to_init->allocator = allocator;
    MACROSPEC_UNICODEPOINT_DFA_STATE_LIST_init(&(to_init->states), allocator, 4);
}

sregex_result MACROSPEC_UNICODEPOINT_DFA_clear(
    MACROSPEC_UNICODEPOINT_DFA *to_clear)
{
    MACROSPEC_UNICODEPOINT_DFA_STATE_LIST_clear(&(to_init->states));
}

sregex_result MACROSPEC_UNICODEPOINT_DFA_create_state(
    MACROSPEC_UNICODEPOINT_DFA *dfa,
    size_t *out_new_state_id)
{

}

sregex_result MACROSPEC_UNICODEPOINT_DFA_set_accept_state(
    MACROSPEC_UNICODEPOINT_DFA *dfa,
    size_t state_id);
sregex_result MACROSPEC_UNICODEPOINT_DFA_add_transition(
    MACROSPEC_UNICODEPOINT_DFA *dfa,
    size_t from_state_id,
    size_t to_state_id,
    MACROSPEC_UNICODEPOINT *codepoint);
sregex_result MACROSPEC_UNICODEPOINT_DFA_add_transitions(
    MACROSPEC_UNICODEPOINT_DFA *dfa,
    size_t from_state_id,
    size_t to_state_id,
    MACROSPEC_UNICODEPOINT *codepoints,
    size_t codepoints_len);
sregex_result MACROSPEC_UNICODEPOINT_DFA_find_matches(
    MACROSPEC_UNICODEPOINT_DFA *dfa,
    MACROSPEC_UNICODEPOINT_ITERATOR *str_iter,
    dfa_match_type match_type,
    MACROSPEC_UNICODEPOINT_PAIR_LIST *out_matches);
