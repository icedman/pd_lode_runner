#include "game_main.h"
#include "map_hint.h"
#include "state.h"

static animations_t guard_animations;

#define RESPAWN_TIME (0.2 * 6)
static state_node_t state_nodes[STATE_LAST] = {
    [STATE_RESPAWN] = {.time = RESPAWN_TIME,
                       .next_state = STATE_IDLE,
                       .valid_next =
                           {
                               STATE_RESPAWN,
                               STATE_LAST,
                           }},
    [STATE_CLIMB_PIT] = {.time = 1,
                         .next_state = STATE_IDLE,
                         .valid_next =
                             {
                                 STATE_CLIMB_PIT,
                                 STATE_LAST,
                             }},
};

static void load(void) {
  state_nodes_init(state_nodes);

  image_t *img = image("assets/images/characters.qoi");
  int o = (12 * 3);
  guard_animations.idle = anim_def(img, vec2i(16, 16), 1, {1 + o});
  guard_animations.walk =
      anim_def(img, vec2i(16, 16), 0.1, {0 + o, 1 + o, 2 + o, 3 + o});
  guard_animations.climb =
      anim_def(img, vec2i(16, 16), 0.1, {12 + o, 13 + o, 14 + o, 15 + o});
  guard_animations.hang =
      anim_def(img, vec2i(16, 16), 0.1, {16 + o, 17 + o, 18 + o, 19 + o});
  guard_animations.fall =
      anim_def(img, vec2i(16, 16), 0.1, {4 + o, 5 + o, 6 + o, 7 + o});
  guard_animations.die = anim_def(
      img, vec2i(16, 16), 0.1, {4 + o, 5 + o, 6 + o, 7 + o, 10, 10, ANIM_STOP});
  guard_animations.respawn =
      anim_def(img, vec2i(16, 16), 0.2,
               {20 + o, 21 + o, 22 + o, 23 + o, 1 + o, ANIM_STOP});

  image_t *tiles = image("assets/images/tileset.qoi");
  guard_animations.dot = anim_def(tiles, vec2i(4, 4), 1, {1});
}

static void init(entity_t *self) {
  self->state.state = STATE_IDLE;
  self->anim = anim(guard_animations.idle);
  self->offset = vec2(4, 2);
  self->size = vec2(8, 14);
  self->friction = vec2(1, 1);
  self->group = ENTITY_GROUP_ENEMY;
  // self->physics = ENTITY_PHYSICS_ACTIVE;
  self->physics = ENTITY_PHYSICS_WORLD;
  self->check_against = ENTITY_GROUP_PLAYER | ENTITY_GROUP_ENEMY;
  self->draw_order = 9;

#ifdef DEBUG_WAYPOINTS
  self->target_entity = entity_spawn(ENTITY_TYPE_OTHER, vec2(0, 0));
  self->target_entity->anim = anim(guard_animations.dot);
  for (int i = 0; i < MAX_WAYPOINTS; i++) {
    self->guard.waypoint_entities[i] =
        entity_spawn(ENTITY_TYPE_OTHER, vec2(0, 0));
  }
#endif
}

static void re_center(entity_t *self) {
  self->pos.x = floor(self->pos.x / 16) * 16 + 4;
  self->pos.y = floor(self->pos.y / 16) * 16 + 2;
}

static void next_waypoint(entity_t *self) {
  if (self->guard.waypoint_count == 0)
    return;
  self->guard.waypoint_origin = self->guard.waypoints[0];
  for (int i = 0; i < self->guard.waypoint_count; i++) {
    self->guard.waypoints[i] = self->guard.waypoints[i + 1];
  }
  self->guard.waypoint_count--;
  self->guard.waypoint_time = 0;
}

static void reverse_waypoints(entity_t *self) {
  if (self->guard.waypoint_count > 0) {
    vec2i_t o = self->guard.waypoint_origin;
    self->guard.waypoint_origin = self->guard.waypoints[0];
    self->guard.waypoints[0] = o;
    self->guard.waypoint_count = 1;
  }
}

static void dump_debug(entity_t *self) {
  printf("guard #%d\n", self->guard.guard_id);
  printf("pos: %.0f %.0f (%.0f %.0f)\n", self->pos.x, self->pos.y,
         floor(self->pos.x / 16), floor(self->pos.y / 16));
  if (self->guard.waypoint_count) {
    vec2i_t o = self->guard.waypoint_origin;
    printf("o (%d, %d)\n", o.x, o.y);
  }
  for (int i = 0; i < self->guard.waypoint_count; i++) {
    vec2i_t wp = self->guard.waypoints[i];
    printf("%d (%d, %d)\n", (i + 1), wp.x, wp.y);
  }
  printf("\n\n");
}

vec2_t random_spawn_pos();

static void kill_and_respawn(entity_t *self) {
  vec2_t pos = random_spawn_pos();
  entity_t *ent =
      entity_spawn(ENTITY_TYPE_GUARD, pos);
  ent->guard.guard_id = self->guard.guard_id;
  ent->guard.has_gold = self->guard.has_gold;
  ent->state.state = STATE_RESPAWN;
  ent->state.timeout = RESPAWN_TIME;
  ent->anim = anim(guard_animations.respawn);
  entity_kill(self);
}

void player_update(entity_t *self, animations_t *anims, float speed);
bool is_on_ladder(entity_t *self, vec2_t offset);
bool is_on_rope(entity_t *self, vec2_t offset);
bool is_on_platform(entity_t *self, vec2_t offset);

static vec2i_t decide_target(entity_t *self) {
  int mid = self->guard.flip ? 4 : 0;
  int currX = floor((self->pos.x + mid) / 16);
  int currY = floor(self->pos.y / 16);

  {
    if (self->current_target.x == 0 && self->current_target.y == 0) {
      self->current_target.x = currX;
      self->current_target.y = currY;
    }
#ifdef DEBUG_WAYPOINTS
    vec2_t pos =
        vec2(self->current_target.x * 16 + 8, self->current_target.y * 16 + 4);
    entity_t *target = self->target_entity;
    target->pos = pos;
#endif
  }

  bool kLeft = false;
  bool kRight = false;
  bool kUp = false;
  bool kDown = false;
  bool kDigLeft = false;
  bool kDigRight = false;

  if (self->guard.waypoint_count > 0) {
    vec2i_t wp = self->guard.waypoints[0];
    vec2i_t origin = self->guard.waypoint_origin;

    // stuck
    if (wp.x == origin.x && wp.y == origin.y) {
      next_waypoint(self);
      if (self->guard.waypoint_count > 0) {
        wp = self->guard.waypoints[0];
        origin = self->guard.waypoint_origin;
      }
    }

    if (wp.x < origin.x) {
      kLeft = true;
    } else if (wp.x > origin.x) {
      kRight = true;
    } else if (wp.y < origin.y) {
      kUp = true;
    } else if (wp.y > origin.y) {
      kDown = true;
    }
  }

  bool moveH = kLeft || kRight;
  bool moveV = kUp || kDown;

  vec2i_t target = self->current_target;
  if (kUp && !moveH) {
    target.y--;
  }
  if (kDown && !moveH) {
    target.y++;
  }
  if (kLeft && !moveV) {
    target.x--;
  }
  if (kRight && !moveV) {
    target.x++;
  }

  if (!moveH) {
    target.x = currX;
    int diff = self->current_target.x - currX;
    if (diff != 0 && !moveV) {
      target.x = currX + (diff > 0 ? 1 : 0);
    }
  }
  if (!moveV) {
    target.y = currY;
    int diff = self->current_target.y - currY;
    if (diff != 0) {
      target.y = currY + (diff > 0 ? 1 : 0);
    }
  }

  return target;
}

static void drop_gold(entity_t *self, vec2_t pos) {
  pos.x = floor(pos.x / 16) * 16;
  pos.y = floor(pos.y / 16) * 16;
  entity_t *ent = entity_spawn(ENTITY_TYPE_GOLD, pos);
  ent->gold.gold_id = self->guard.gold_id;
  self->guard.has_gold = false;
}

static void update(entity_t *self) {
  vec2_t pos = self->pos;
  state_e current_state = self->state.state;
  state_e next_state = current_state;
  float timeout = self->state.timeout;

  // force beeline
  bool should_find_new_path = (self->guard.waypoint_count == 0);
  if (!should_find_new_path) {
    self->guard.waypoint_time += engine.tick;
    if (self->guard.waypoint_time > RETHINK_TIME) {
      self->guard.waypoint_time = 0;
      should_find_new_path = true;
    }
  }

  if (self->idle_time > STUCK_TIME) {
    self->idle_time = 0;
    if (self->guard.waypoint_count != 0) {
      should_find_new_path = true;
      // printf("suck!? %d %f\n", self->guard.guard_id, self->idle_time);
    } else {
      printf("no way points?!\n");
    }
  }

  // AI
// force a bee line
#if 1
  if (!should_find_new_path) {
    int sx = floor(self->pos.x / 16);
    int sy = floor(self->pos.y / 16);
    int tx = floor(g.player->pos.x / 16);
    int ty = floor(g.player->pos.y / 16);
    // on the same vertical path
    if (sx == tx) {
      map_tile_t *gt = get_map_hint_tile(sx, sy);
      map_tile_t *pt = get_map_hint_tile(tx, ty);
      if (gt && pt && gt->fall_id == pt->fall_id) {
        should_find_new_path = true;
      } else if (gt && pt && gt->climb_id == pt->climb_id) {
        should_find_new_path = true;
      }
      // on the same platform
    } else if (sy == ty) {
      map_tile_t *gt = get_map_hint_tile(sx, sy);
      map_tile_t *pt = get_map_hint_tile(tx, ty);
      if (gt && pt && gt->platform_id == pt->platform_id) {
        should_find_new_path = true;
      }
    }
  }
#endif

  // todo randomly drop gold
  if (self->guard.has_gold) {
    self->guard.gold_time += engine.tick;
    if (self->guard.gold_time > GOLD_HOLD_TIME) {
      if (is_on_platform(self, vec2(0, 16)) &&
          !is_on_ladder(self, vec2(0, 0)) && !is_on_rope(self, vec2(0, 0))) {
        drop_gold(self, self->pos);
      }
    }
  }

  // let pit decide where we go
  if (self->guard.trapped) {
    should_find_new_path = false;
  }

  if (should_find_new_path) {
    self->guard.waypoint_count = 0;
  }

  if (self->guard.waypoint_count == 0) {
    self->guard.waypoint_time = 0;
    find_path(self, g.player);
#ifdef ENABLE_ASTAR
    if (self->guard.waypoint_count == 0) {
      find_astar_path(self, g.player);
    }
#endif
  }

  if (input_pressed(A_ENTER)) {
    dump_debug(self);
    //   timeout = RESPAWN_TIME;
    //   next_state = STATE_RESPAWN;
    //   self->state.timeout = timeout;
    //   self->state.state = STATE_RESPAWN;
  }

  self->next_target = decide_target(self);

  float speed = 30;
  if (self->guard.slow_time > 0) {
    self->guard.slow_time -= engine.tick;
    speed = 8;
  }
  player_update(self, &guard_animations, speed);

  vec2i_t current_pos = vec2i(floor(self->pos.x / 16), floor(self->pos.y / 16));

  if (self->state.state == STATE_DIE) {
    kill_and_respawn(self);
    return;
  }

  state_commit(&self->state, state_nodes, next_state, timeout);

#ifdef DEBUG_WAYPOINTS
  vec2i_t p1 = current_pos;
  // if (self->guard.guard_id == 0)
  for (int i = 0; i < MAX_WAYPOINTS; i++) {
    entity_t *ent = self->guard.waypoint_entities[i];
    if (i >= self->guard.waypoint_count) {
      ent->anim.def = NULL;
      continue;
    }
    vec2i_t p2 = self->guard.waypoints[i];
    ent->anim = anim(guard_animations.dot);
    ent->pos = vec2(p2.x * 16 + 8, p2.y * 16 + 4);
    p1 = p2;
  }
#endif

  if (self->guard.waypoint_count > 0) {
    vec2i_t wp = self->guard.waypoints[0];
    if (wp.x == current_pos.x && wp.y == current_pos.y) {
      vec2_t target_pos = vec2(wp.x * 16 + 4, wp.y * 16 + 2);
      float dist = vec2_dist(target_pos, self->pos);
      if (dist < 4) {
        next_waypoint(self);
      }
    }
  }
}

static void draw(entity_t *self, vec2_t viewport) {
  entity_base_draw(self, viewport);

  // char tmp[64];
  // sprintf(tmp, "%d", self->guard.guard_id);
  // draw_text(tmp, self->pos.x - viewport.x + 8, self->pos.y - viewport.y - 8,
  // 0);

  if (self->guard.has_gold) {
    draw_text("!", self->pos.x - viewport.x - 2, self->pos.y - viewport.y - 17,
              0);
  }
}

static void damage(entity_t *self, entity_t *other, float damage) {
  if (other->type == ENTITY_TYPE_PIT) {
    // fall on pit.. drop gold
    if (self->guard.has_gold) {
      vec2_t pos = other->pit.pos;
      pos.y -= 18;
      drop_gold(self, pos);
    }
    if (other->pit.time >= TRAP_KILL_TIME) {
      if (self->state.state != STATE_DIE) {
        self->state.state = STATE_DIE;
      }
    }
  }
}

static void touch(entity_t *self, entity_t *other) {
  if (other->type == ENTITY_TYPE_GUARD) {
    //
    if (other->guard.guard_id > self->guard.guard_id) {
      self->guard.slow_time = SLOW_TIME;
    }
    return;
  }
  if (self->pos.y > other->pos.y + 2) {
    if (self->state.state != STATE_CLIMB) {
      return;
    }
  }
  if (self->state.state == STATE_RESPAWN) {
    return;
  }
  entity_damage(other, self, 0);
}

entity_vtab_t entity_vtab_guard = {.load = load,
                                   .init = init,
                                   .update = update,
                                   .draw = draw,
                                   .damage = damage,
                                   .touch = touch};