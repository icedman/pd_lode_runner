#include "game_main.h"
#include "map_hint.h"
#include "state.h"

// #define TEST_NO_DEATH

static sound_source_t *snd_walk;
static sound_source_t *snd_fall;
static sound_source_t *snd_climb;
static sound_source_t *snd_hang;
static sound_source_t *snd_dig;
static sound_source_t *snd_die;

static animations_t player_animations;

static state_node_t state_nodes[STATE_LAST] = {
    [STATE_DIG] = {.time = 0.65,
                   .next_state = STATE_IDLE,
                   .valid_next =
                       {
                           STATE_DIG,
                           STATE_LAST,
                       }},
    [STATE_DIE] = {.time = 1,
                   .next_state = STATE_DIE,
                   .valid_next =
                       {
                           STATE_DIE,
                           STATE_LAST,
                       }},
};

static void load(void) {
  state_nodes_init(state_nodes);

  image_t *img = image("assets/images/characters.qoi");

  player_animations.idle = anim_def(img, vec2i(16, 16), 1, {1});
  player_animations.walk = anim_def(img, vec2i(16, 16), 0.1, {0, 1, 2, 3});
  player_animations.climb = anim_def(img, vec2i(16, 16), 0.1, {12, 13, 14, 15});
  player_animations.hang = anim_def(img, vec2i(16, 16), 0.1, {16, 17, 18, 19});
  player_animations.fall = anim_def(img, vec2i(16, 16), 0.2, {4, 5, 6, 7});
  player_animations.dig =
      anim_def(img, vec2i(16, 16), 0.2, {24, 25, 26, 27, ANIM_STOP});
  player_animations.die = anim_def(
      img, vec2i(16, 16), 0.2,
      {28, 29, 30, 31, 32, 33, 34, 35, 10, 10, 10, 10, 10, 10, ANIM_STOP});
  player_animations.shout = anim_def(img, vec2i(16, 16), 0.5, {20, 21, 22, 23});

  image_t *tiles = image("assets/images/tileset.qoi");
  player_animations.dot = anim_def(tiles, vec2i(4, 4), 1, {1});

  snd_walk = sound_source("assets/sfx/step1.qoa");
  snd_fall = sound_source("assets/sfx/fall.qoa");
  snd_climb = sound_source("assets/sfx/ladder1.qoa");
  snd_hang = sound_source("assets/sfx/pole1.qoa");
  snd_dig = sound_source("assets/sfx/dig.qoa");
  snd_die = sound_source("assets/sfx/death.qoa");
}

static void init(entity_t *self) {
  self->state.state = STATE_IDLE;
  self->anim = anim(player_animations.idle);
  self->physics = ENTITY_PHYSICS_ACTIVE;
  self->offset = vec2(4, 2);
  self->size = vec2(8, 14);
  self->friction = vec2(1, 1);
  self->group = ENTITY_GROUP_PLAYER;
  self->draw_order = 10;

#ifdef DEBUG_WAYPOINTS
  self->target_entity = entity_spawn(ENTITY_TYPE_OTHER, vec2(0, 0));
  self->target_entity->anim = anim(player_animations.dot);
#endif
}

static bool is_solid_at_px(entity_t *self, vec2_t offset) {
  map_t *col = engine.collision_map;
  return map_tile_at_px(col, vec2_add(self->pos, offset)) != 0;
}

void player_shout(entity_t *self) {
  self->anim = anim(player_animations.shout);
}

bool is_on_ladder(entity_t *self, vec2_t offset) {
  map_t *bg = engine.background_maps[0];
  int tileLedge = map_tile_at_px(bg, vec2_add(self->pos, offset));
  offset.y += 4;
  int tileLedge2 = map_tile_at_px(bg, vec2_add(self->pos, offset));

  // self->player.gold_collected = g.level_golds;

  if (!g.level_complete && (g.level_golds <= self->player.gold_collected)) {
    if (self->type == ENTITY_TYPE_PLAYER && self->pos.y < -8 + 16) {
      g.level_complete = true;
      // printf("level complete! %d %d\n", g.level_golds,
      // self->player.gold_collected);
      engine_set_scene(&scene_level_complete);
    }
  }

  return mapChars[tileLedge] == 'S' || mapChars[tileLedge] == 'H' ||
         mapChars[tileLedge2] == 'H';
}

bool is_on_platform(entity_t *self, vec2_t offset) {
  map_t *bg = engine.background_maps[0];
  int tileLedge = map_tile_at_px(bg, vec2_add(self->pos, offset));
  return mapChars[tileLedge] == '#' || mapChars[tileLedge] == '@';
}

bool is_on_pit_top(entity_t *self, vec2_t offset) {
  map_t *bg = engine.background_maps[0];
  int tileLedge = map_tile_at_px(bg, vec2_add(self->pos, offset));

  // escaped guards can't fall on the same pit
  if (self->type == ENTITY_TYPE_GUARD) {
    vec2_t p = vec2_add(self->pos, offset);
    map_tile_t *mt = get_map_hint_tile(p.x / 16, p.y / 16);
    if (self->guard.trapped_pit_id == mt->pit_id) {
      return false;
    }
  }

  return tileLedge == 11;
}

bool is_on_pit_top_with_enemy(entity_t *self, vec2_t offset) {
  map_t *bg = engine.background_maps[0];
  int tileLedge = map_tile_at_px(bg, vec2_add(self->pos, offset));
  return tileLedge == 22;
}

bool is_on_rope(entity_t *self, vec2_t offset) {
  map_t *bg = engine.background_maps[0];
  int tileLedge = map_tile_at_px(bg, vec2_add(self->pos, offset));
  return mapChars[tileLedge] == '-';
}

static void re_center(entity_t *self) {
  self->pos.x = floor(self->pos.x / 16) * 16 + 4;
  self->pos.y = floor(self->pos.y / 16) * 16 + 2;
}

static vec2i_t decide_target(entity_t *self) {
  int mid = self->player.flip ? 4 : 0;
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

  state_e current_state = self->state.state;

  if (self->state.state == STATE_DIE) {
    return self->current_target;
  }

  bool kLeft = input_state(A_LEFT);
  bool kRight = input_state(A_RIGHT);
  bool kUp = input_state(A_UP);
  bool kDown = input_state(A_DOWN);
  bool kDigLeft = input_state(A_DIG_LEFT);
  bool kDigRight = input_state(A_DIG_RIGHT);

  if (current_state != STATE_DIG) {
    if (kRight || kDigRight) {
      self->player.flip = true;
    } else if (kLeft || kDigLeft) {
      self->player.flip = false;
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

void player_update(entity_t *self, animations_t *anims, float speed) {
  map_t *bg = engine.background_maps[0];
  map_t *col = engine.collision_map;

  if (self->state.state == STATE_RESPAWN || self->state.state == STATE_DIE ||
      g.level_complete) {
    speed = 0;
  }

  bool was_on_ladder = is_on_ladder(self, vec2(0, 0));
  bool was_on_ladder_top = is_on_ladder(self, vec2(0, 8));
  bool was_on_rope = is_on_rope(self, vec2(0, 0));
  bool was_on_platform = is_on_platform(self, vec2(0, 16));
  bool was_on_pit_top = is_on_pit_top(self, vec2(0, 16));
  bool was_on_pit = is_on_pit_top(self, vec2(0, 0));

  bool is_falling =
      !was_on_ladder && !was_on_ladder_top && !was_on_rope && !was_on_platform;
  bool is_pit_falling = was_on_pit_top && !was_on_rope && !was_on_ladder;

  int mid = self->player.flip ? 4 : 0;
  int currX = floor((self->pos.x + mid) / 16);
  int currY = floor(self->pos.y / 16);

  vec2i_t target = self->next_target;
  vec2_t current_pos = self->pos;

  // validate
  map_tile_t *mt = get_map_hint_tile(target.x, target.y);
  map_tile_t *mc = get_map_hint_tile(currX, currY);

  if (is_falling && mc->platform_id == 0 && mc->climb_id == 0) {
    target.x = currX;
    target.y = currY + 1;
  }

  bool validTarget = true;
  if (validTarget && mt) {
    if (mt->platform_id == 0 && target.x != currX) {
      validTarget = false;
    }
    if (mt->climb_id == 0 && target.y < currY) {
      validTarget = false;
    }
  }
  if (!validTarget) {
    if (mc->platform_id != 0 && map_tile_at(col, target) == 0 &&
        target.y == currY) {
      validTarget = true;
    }
  }
  // can't jump to a hanging ladder
  if (validTarget && map_tile_at(col, target) != 0) {
    validTarget = false;
  }
  // on S ladder
  if (!validTarget && map_tile_at(bg, target) == 6 && target.y < currY) {
    validTarget = true;
  }

  // on pit ... moving horizontally
  if (!validTarget && self->type == ENTITY_TYPE_PLAYER && was_on_pit) {
    if (input_state(A_LEFT)) {
      target.x = currX - 1;
    } else if (input_state(A_RIGHT)) {
      target.x = currX + 1;
    }
    int c = map_tile_at(col, target);
    int b = map_tile_at(bg, target);
    if (c == 0 && b == 11) {
      validTarget = true;
      target.y = currY;
    }
  }

  // at exit ladders?
  if (!validTarget && mc->climb_id == 0 && was_on_ladder && target.y != currY) {
    validTarget = true;
  }

  // climbing out is always valid
  if (self->type == ENTITY_TYPE_GUARD && self->state.state == STATE_CLIMB_PIT) {
    target = self->guard.waypoints[0];

    // climbing out is like on ladder
    validTarget = true;
    was_on_ladder = true;
    is_falling = false;
    is_pit_falling = false;
  }

  if (!validTarget) {
    target = self->current_target;
  }

  // corrections
  if (is_pit_falling) {
    target.x = currX;
    target.y = currY + 1;
  }
  if (!was_on_ladder && target.y < currY) {
    target.y = currY;
  }

  self->current_target = target;

  // speed adjust
  if (self->anim.def == anims->hang) {
    speed *= 0.6;
  }
  if (self->anim.def == anims->climb) {
    speed *= 0.6;
  }
  if (self->anim.def == anims->fall) {
    speed *= 1.2;
  }

  bool didMoveX = true;
  bool didMoveY = true;

  if (self->state.state == STATE_DIG) {
    self->current_target.x = currX;
  }

  if (self->current_target.x < currX) {
    self->pos.x -= speed * engine.tick;
  } else if (self->current_target.x > currX) {
    self->pos.x += speed * engine.tick;
  } else if (self->current_target.x == currX) {
    // to center
    vec2_t center = vec2(currX * 16 + 4, currY * 16 + 2);
    if (center.x < self->pos.x) {
      self->pos.x -= speed * engine.tick;
    } else if (center.x > self->pos.x) {
      self->pos.x += speed * engine.tick;
    }
    if (abs(center.x - self->pos.x) < 1) {
      self->pos.x = center.x;
      didMoveX = false;
    }
  }
  if (self->current_target.y < currY) {
    self->pos.y -= speed * engine.tick;
  } else if (self->current_target.y > currY) {
    self->pos.y += speed * engine.tick;
  } else if (self->current_target.y == currY) {
    // to center
    vec2_t center = vec2(currX * 16 + 4, currY * 16 + 2);
    if (center.y < self->pos.y) {
      self->pos.y -= speed * engine.tick;
    } else if (center.y > self->pos.y) {
      self->pos.y += speed * engine.tick;
    }
    if (abs(center.y - self->pos.y) < 1) {
      self->pos.y = center.y;
      didMoveY = false;
    }
  }

  bool moveH = currX != target.x;
  bool moveV = currY != target.y;

  if (didMoveX || didMoveY) {
    self->idle_time = 0;
  } else {
    self->idle_time += engine.tick;
  }

  // to resolve collisions
  // self->gravity = 0;
  // entity_base_update(self);

  anim_def_t *next_anim = self->anim.def;
  if (moveH) {
    next_anim = anims->walk;
    if (was_on_rope) {
      next_anim = anims->hang;
    } else {
    }
    self->player.flip = (currX < target.x);
  } else if (moveV) {
    next_anim = anims->fall;
    if (was_on_ladder) {
      next_anim = anims->climb;
    }
  } else {
    if (!was_on_rope || was_on_ladder_top) {
      next_anim = anims->idle;
    }
    if (was_on_ladder && !was_on_platform) {
      next_anim = anims->climb;
    }
  }

  // die
  if (self->state.state == STATE_DIE) {
    next_anim = anims->die;
    if (self->anim.def == anims->die && anim_looped(&self->anim)) {
      engine_set_scene(&scene_level_start);
      return;
    }
  }
  // respawn
  if (self->state.state == STATE_RESPAWN) {
    next_anim = anims->respawn;
  }
  // dig
  if (self->state.state == STATE_DIG) {
    next_anim = anims->dig;
  }
  // trapped
  if (self->type == ENTITY_TYPE_GUARD && self->guard.trapped &&
      self->state.state != STATE_CLIMB_PIT) {
    next_anim = anims->fall;
  }

  // stop animation
  if (next_anim == anims->climb && !moveV &&
      self->state.state != STATE_CLIMB_PIT) {
    self->anim.def = NULL;
  }
  if (next_anim == anims->hang && !moveH) {
    self->anim.def = NULL;
  }

  if (next_anim != self->anim.def) {
    self->anim = anim(next_anim);
  }
  self->anim.flip_x = self->player.flip;
}

static void update(entity_t *self) {
  vec2_t pos = self->pos;
  state_e current_state = self->state.state;
  state_e next_state = current_state;
  float timeout = self->state.timeout;

  self->next_target = decide_target(self);
  player_update(self, &player_animations, 60);

  bool kDigLeft = input_state(A_DIG_LEFT);
  bool kDigRight = input_state(A_DIG_RIGHT);

  bool was_on_ladder = is_on_ladder(self, vec2(0, 0));
  bool was_on_ladder_top = is_on_ladder(self, vec2(0, 8));
  bool was_on_rope = is_on_rope(self, vec2(0, 0));
  bool was_on_platform = is_on_platform(self, vec2(0, 16));
  bool was_on_ladder_platform = is_on_ladder(self, vec2(0, 16));
  bool was_on_pit_top_with_enemy = is_on_pit_top_with_enemy(self, vec2(0, 16));

  bool can_dig =
      was_on_ladder_platform || was_on_platform || was_on_pit_top_with_enemy;
  if (can_dig && (kDigLeft || kDigRight) && self->idle_time > 0) {
    vec2_t dig_pos = self->pos;
    int mid = self->player.flip ? 4 : 0;
    dig_pos.x = (floor((dig_pos.x + mid) / 16) * 16) + 0;
    dig_pos.y += 14;
    dig_pos.x += self->player.flip ? 16 : -16;
    entity_t *pit = entity_spawn(ENTITY_TYPE_PIT, dig_pos);
    re_center(self);
    self->vel.x = 0;
    next_state = STATE_DIG;
  }

  state_commit(&self->state, state_nodes, next_state, timeout);

  // sfx
  if (self->anim.def == player_animations.walk) {
    sound_play(snd_walk);
  } else if (self->anim.def == player_animations.climb) {
    if (input_state(A_UP) || input_state(A_DOWN)) {
      sound_play(snd_climb);
    }
  } else if (self->anim.def == player_animations.hang) {
    if (input_state(A_LEFT) || input_state(A_RIGHT)) {
      sound_play(snd_hang);
    }
  }

  // pause
  if (self->anim.def == player_animations.dig) {
    sound_play(snd_dig);
  } else {
    sound_pause(sound(snd_dig));
  }
  if (self->anim.def == player_animations.fall) {
    sound_play(snd_fall);
  } else {
    sound_pause(sound(snd_fall));
  }
}

static void draw(entity_t *self, vec2_t viewport) {
  entity_base_draw(self, viewport);
}

static void damage(entity_t *self, entity_t *other, float damage) {
  bool will_die = false;
  if (other->type == ENTITY_TYPE_PIT) {
    if (other->pit.time >= TRAP_KILL_TIME) {
      will_die = true;
    }
  }

  if (other->type == ENTITY_TYPE_GUARD) {
    will_die = true;
  }

  // kill this player
#ifndef TEST_NO_DEATH
  if (will_die) {
    if (self->state.state != STATE_DIE) {
      self->state.state = STATE_DIE;
      self->physics = ENTITY_PHYSICS_WORLD;
      sound_play(snd_die);
    }
  }
#endif
}

static void touch(entity_t *self, entity_t *other) {
  // entity_damage(other, self, 10);
}

entity_vtab_t entity_vtab_player = {.load = load,
                                    .init = init,
                                    .update = update,
                                    .draw = draw,
                                    .damage = damage,
                                    .touch = touch};