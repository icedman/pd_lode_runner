#include "game_main.h"

static anim_def_t *gold;

extern const char *mapChars;
static sound_source_t *snd_gold;
static sound_source_t *snd_everygold;

static int gold_id = 1;

void show_exit_ladder();

static void load(void) {
  image_t *img = image("assets/images/tileset.qoi");
  gold = anim_def(img, vec2i(16, 16), 1, {6});

  snd_gold = sound_source("assets/sfx/gold.qoa");
  snd_everygold = sound_source("assets/sfx/everygold.qoa");
}

static void init(entity_t *self) {
  self->size = vec2(14, 14);
  self->anim = anim(gold);
  self->check_against = ENTITY_GROUP_PLAYER | ENTITY_GROUP_ENEMY;
  self->gold.gold_id = gold_id++;
}

static void update(entity_t *self) { entity_base_update(self); }

static void draw(entity_t *self, vec2_t viewport) {
  entity_base_draw(self, viewport);
}

static void damage(entity_t *self, entity_t *other, float damage) {}

static void touch(entity_t *self, entity_t *other) {
  // only one may touch!
  if (self->anim.def == NULL) {
    return;
  }

  if (other->type == ENTITY_TYPE_PLAYER) {
    self->anim.def = NULL;
    other->player.gold_collected++;
    entity_kill(self);

    if (g.exit_ladder_count > 0 && g.player &&
        g.player->player.gold_collected >= g.level_golds && g.level_golds > 0) {
      show_exit_ladder();
      sound_play(snd_everygold);
    } else {
      sound_play(snd_gold);
    }
  }

  if (other->type == ENTITY_TYPE_GUARD && !other->guard.trapped &&
      !other->guard.has_gold) {
    if (other->state.state == STATE_FALL) {
      return;
    }

    // cant take the gold again after dropping it
    if (other->guard.gold_id == self->gold.gold_id) {
      return;
    }

    self->anim.def = NULL;
    entity_kill(self);

    other->guard.gold_id = self->gold.gold_id;
    other->guard.has_gold = true;
    other->guard.gold_time = 0;
  }
}

entity_vtab_t entity_vtab_gold = {
    .load = load, .init = init, .update = update, .draw = draw, .touch = touch};