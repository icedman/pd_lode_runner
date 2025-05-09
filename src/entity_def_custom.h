#include "state.h"

#define MAX_WAYPOINTS 32

#define ENTITY_TYPES(TYPE)                                                     \
  TYPE(ENTITY_TYPE_OTHER, other)                                               \
  TYPE(ENTITY_TYPE_GUARD, guard)                                               \
  TYPE(ENTITY_TYPE_GOLD, gold)                                                 \
  TYPE(ENTITY_TYPE_PIT, pit)                                                   \
  TYPE(ENTITY_TYPE_TRAP, trap)                                                 \
  TYPE(ENTITY_TYPE_PLAYER, player)

// All entity types share the same struct. Calling ENTITY_DEFINE() defines that
// struct with the fields required by high_impact and the additional fields
// specified here.

ENTITY_DEFINE(
    // Entity private data
    state_t state;

    vec2i_t next_target; vec2i_t current_target; entity_t * target_entity;
    float idle_time;

    int last_platform_id; int last_fall_id; void *last_platform_tile;

    union {
      struct {
        bool flip;
        int gold_collected;
      } player;

      struct {
        bool flip;

        int guard_id;

        int gold_id;
        float gold_time;
        bool has_gold;

        bool trapped;
        int trapped_pit_id;

        vec2i_t waypoint_origin;
        vec2i_t waypoints[MAX_WAYPOINTS];
        int waypoint_count;
        float waypoint_time;

        float slow_time;

        // debug purposes
        entity_t *waypoint_entities[MAX_WAYPOINTS];
      } guard;

      struct {
        float time;
        vec2_t pos;
        int tileIdx;

        int pit_id;

        vec2i_t pit_xy;
        vec2i_t pit_top_xy;
        vec2i_t pit_bottom_xy;

        float trap_time;
        entity_t *trapped_enemy;
      } pit;

      struct {
        float time;
        int gold_id;
      } gold;

      struct {
        int other;
      } other;
    };);

// The entity_message_t is used with the entity_message() function. You can
// extend this as you wish.

typedef enum {
  EM_INVALID,
  EM_ACTIVATE,
} entity_message_t;
