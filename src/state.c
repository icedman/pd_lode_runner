#include "state.h"
#include "../engine/engine.h"

void state_nodes_init(state_node_t *nodes) {
  for (int i = 0; i < STATE_LAST; i++) {
    for (int j = 0; j < STATE_LAST; j++) {
      if (nodes[i].valid_next[j] == STATE_LAST)
        break;
      nodes[i].valid[nodes[i].valid_next[j]] = true;
    }
  }
}

bool state_is_next_valid(state_node_t *nodes, state_e from, state_e to) {
  return nodes[from].valid[to];
}

state_e state_get_next(state_node_t *nodes, state_e from, state_e to) {
  return state_is_next_valid(nodes, from, to) ? to : from;
}

bool state_commit(state_t *state, state_node_t *nodes, state_e next_state,
                  float timeout) {
  state_node_t *ns = &nodes[next_state];
  state_e current_state = state->state;
  state->timeout = timeout;
  if (state->state != next_state) {
    state->state = next_state;
    state->timeout = ns->time;
    state->ticks = 0;
  } else {
    state->ticks += engine.tick;
    state->timeout -= engine.tick;
    if (state->timeout <= 0) {
      state->state = ns->next_state;
      state->timeout = 0;
      state->ticks = 0;
    }
  }
  if (current_state != state->state) {
    state->previous_state = current_state;
    return true;
  }
  return false;
}