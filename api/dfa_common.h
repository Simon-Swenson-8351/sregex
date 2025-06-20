#pragma once

#define DFA_START_STATE_ID 0

typedef enum dfa_state_type
{
    DFA_STATE_TYPE_REJECT,
    DFA_STATE_TYPE_ACCEPT,

    DFA_STATE_TYPE__LEN
} dfa_state_type;

typedef enum dfa_match_type
{
    DFA_MATCH_TYPE_ANYWHERE,
    DFA_MATCH_TYPE_PREFIX,
    DFA_MATCH_TYPE_EXACT,

    DFA_MATCH_TYPE__LEN
} dfa_match_type;
