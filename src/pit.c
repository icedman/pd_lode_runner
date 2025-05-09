#include "game_main.h"
#include "map_hint.h"

static anim_def_t *erode;
static anim_def_t *empty;
static anim_def_t *restore;

extern const char *mapChars;

static int pit_id = 1;

static void load(void) {
  image_t *img = image("assets/images/tileset.qoi");
  erode = anim_def(img, vec2i(16, 16), 0.2,
                   {11, 22, 33, 44, 55, 20, 20, ANIM_STOP});
  empty = anim_def(img, vec2i(16, 16), 1.2, {20, 20, 20, 20, 20, ANIM_STOP});
  restore =
      anim_def(img, vec2i(16, 16), 0.2, {56, 45, 34, 23, 12, 0, 0, ANIM_STOP});
}

static void init(entity_t *self) {
  self->size = vec2(16, 16);
  map_t *col = engine.collision_map;
  map_t *bg = engine.background_maps[0];
  vec2_t digPos = self->pos;
  int idx = map_tile_at_px(bg, digPos);
  char ch = mapChars[idx];
  if (idx == 0 || ch != '#') {
    return;
  }
  map_set_tile_at_px(bg, digPos, 11);
  map_set_tile_at_px(col, digPos, 0);
  self->pit.pit_id = pit_id++;
  self->pit.tileIdx = idx;
  self->pit.time = 0;
  self->pit.pos = digPos;
  self->pit.pos.y += 2;
  self->anim = anim(erode);
  self->check_against = ENTITY_GROUP_PLAYER | ENTITY_GROUP_ENEMY;

  self->pit.pit_xy = vec2i(self->pos.x / 16, self->pos.y / 16);
  self->pit.pit_top_xy = self->pit.pit_xy;
  self->pit.pit_top_xy.y--;
  self->pit.pit_bottom_xy = self->pit.pit_xy;
  self->pit.pit_bottom_xy.y++;

  map_tile_t *mt = get_map_hint_tile(self->pit.pit_xy.x, self->pit.pit_xy.y);
  if (mt) {
    mt->pit_id = self->pit.pit_id;
  }
}

static void update(entity_t *self) {
  // cannot break space
  if (self->pit.tileIdx == 0) {
    entity_kill(self);
    return;
  }

  map_t *col = engine.collision_map;
  map_t *bg = engine.background_maps[0];

  self->pit.time += engine.tick;
  if (anim_looped(&self->anim)) {
    if (self->anim.def == erode) {
      self->anim = anim(empty);
    } else if (self->anim.def == empty) {
      self->anim = anim(restore);
    } else {
      // entity_kill(self);
      // restore collisions prior
      map_set_tile_at_px(bg, self->pit.pos, self->pit.tileIdx);
      map_set_tile_at_px(col, self->pit.pos, 1);
      // kill self on the next frame
      self->pit.tileIdx = 0;
      return;
    }
  }

  if (self->pit.trapped_enemy) {
    self->pit.trap_time += engine.tick;
    float time = self->pit.trap_time;
    entity_t *guard = self->pit.trapped_enemy;
    if (guard->state.state == STATE_DIE) {
      return;
    }

    map_set_tile_at_px(col, self->pit.pos, 1);

    if (time < ESCAPE_TIME) {
      guard->physics = ENTITY_PHYSICS_WORLD;

      if (time > 0.2) {
        guard->pos.x = self->pos.x + 4;
        guard->pos.y = self->pos.y + 2;
      }

      // fall... todo .. don't fall beyond
    } else if (time >= ESCAPE_TIME && time <= ESCAPE_CLIMB_TIME) {
      guard->guard.waypoint_count = 1;
      guard->guard.waypoint_origin = self->pit.pit_bottom_xy;
      guard->guard.waypoints[0] = self->pit.pit_top_xy;
      guard->state.state = STATE_CLIMB_PIT;
    } else if (time > ESCAPE_CLIMB_TIME) {
      map_set_tile_at_px(bg, self->pit.pos, 11);
      map_set_tile_at_px(col, self->pit.pos, 0);
      guard->state.state = STATE_IDLE;
      guard->guard.waypoint_count = 0;
      guard->guard.trapped = false;
      // guard->physics = ENTITY_PHYSICS_ACTIVE;
      guard->physics = ENTITY_PHYSICS_WORLD;
      guard->check_against = ENTITY_GROUP_PLAYER | ENTITY_GROUP_ENEMY;
      self->pit.trapped_enemy = NULL;
    }
  }
}

static void draw(entity_t *self, vec2_t viewport) {
  entity_base_draw(self, viewport);
}

static void damage(entity_t *self, entity_t *other, float damage) {
  // entity_base_damage(self, other, damage);
}

static void touch(entity_t *self, entity_t *other) {
  if (self->pit.tileIdx == 0) {
    return;
  }

  map_t *col = engine.collision_map;
  map_t *bg = engine.background_maps[0];
  if (self->pos.y > other->pos.y + 4) {
    return;
  }

  if (!self->pit.trapped_enemy) {
    if (other->type == ENTITY_TYPE_GUARD &&
        other->guard.trapped_pit_id != self->pit.pit_id) {
      if (self->pos.y > other->pos.y + 14) {
        other->pos.y = floor(other->pos.y / 16) * 16 + 2;
      }
      other->physics = ENTITY_PHYSICS_WORLD;
      other->check_against = 0;
      other->guard.trapped = true;
      other->guard.trapped_pit_id = self->pit.pit_id;
      self->pit.trap_time = 0;
      self->pit.trapped_enemy = other;
      map_set_tile_at_px(col, self->pit.pos, 1);
      map_set_tile_at_px(bg, self->pit.pos, 22);
    }
  }

  if (other->type == ENTITY_TYPE_PLAYER) {
    if (!input_state(A_LEFT) && !input_state(A_RIGHT)) {
      other->pos.x = floor(other->pos.x / 16) * 16 + 4; // recenter
    }
  } else if (other->type == ENTITY_TYPE_GUARD) {
    other->pos.x = floor(other->pos.x / 16) * 16 + 4; // recenter
  }

  other->vel.x = 0;
  if (self->pit.time >= TRAP_KILL_TIME) {
    // release before this burns us
    self->pit.trapped_enemy = NULL;
  }
  entity_damage(other, self, self->pit.time);
}

entity_vtab_t entity_vtab_pit = {.load = load,
                                 .init = init,
                                 .update = update,
                                 .draw = draw,
                                 .damage = damage,
                                 .touch = touch};