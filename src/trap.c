#include "game_main.h"

static anim_def_t *brick;
static anim_def_t *trap;

extern const char *mapChars;
extern float wait_timeout;

static void load(void) {
  image_t *img = image("assets/images/tileset.qoi");
  brick = anim_def(img, vec2i(16, 16), 1, {0});
  trap = anim_def(img, vec2i(16, 16), 0.1, {0, 0, 4, 4, ANIM_STOP});
}

static void init(entity_t *self) {
  self->size = vec2(14, 14);
  self->anim = anim(brick);
  self->check_against = ENTITY_GROUP_PLAYER;
}

static void update(entity_t *self) { entity_base_update(self); }

static void draw(entity_t *self, vec2_t viewport) {
  entity_base_draw(self, viewport);
}

static void damage(entity_t *self, entity_t *other, float damage) {}

static void touch(entity_t *self, entity_t *other) {
  if (other->type == ENTITY_TYPE_PLAYER && self->anim.def != trap) {
    self->anim = anim(trap);
  }
}

entity_vtab_t entity_vtab_trap = {
    .load = load, .init = init, .update = update, .draw = draw, .touch = touch};