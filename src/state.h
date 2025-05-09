#ifndef ENTITY_STATE_H
#define ENTITY_STATE_H

#include "types.h"

typedef enum {
  STATE_IDLE = 0,
  STATE_WALK,
  STATE_CLIMB,
  STATE_HANG,
  STATE_DIG,
  STATE_FALL,
  STATE_CLIMB_PIT,
  STATE_DIE,
  STATE_RESPAWN,
  STATE_LAST
} state_e;

typedef struct {
  float time;
  state_e next_state;
  state_e valid_next[STATE_LAST];
  bool valid[STATE_LAST];
} state_node_t;

typedef struct {
  state_e state;
  state_e previous_state;
  float timeout;
  float ticks;
} state_t;

void state_nodes_init(state_node_t *nodes);
bool state_is_next_valid(state_node_t *nodes, state_e from, state_e to);
state_e state_get_next(state_node_t *nodes, state_e from, state_e to);
bool state_commit(state_t *state, state_node_t *nodes, state_e next_state,
                  float timeout);

#endif // ENTITY_STATE_H