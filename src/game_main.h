#ifndef MAIN_H
#define MAIN_H

#include "animation.h"
#include "camera.h"
#include "engine.h"
#include "entity.h"
#include "font.h"
#include "input.h"
#include "noise.h"
#include "platform.h"
#include "render.h"
#include "sound.h"
#include "types.h"
#include "utils.h"

// #define TEST_MAP_LOADING
// #define TEST_EXIT_LADDER
// #define DEBUG_WAYPOINTS

#define ENABLE_ASTAR

// pit
#define TRAP_TIME 2.0
#define TRAP_KILL_TIME 6.0
#define ESCAPE_TIME (TRAP_TIME + 0.8)
#define ESCAPE_CLIMB_TIME (ESCAPE_TIME + 1.0)

// guard thinking
#define RETHINK_TIME 4
#define GOLD_HOLD_TIME 6
#define SLOW_TIME 0.5
#define STUCK_TIME 2.0

// -----------------------------------------------------------------------------
// Button actions

typedef enum {
  A_UP,
  A_DOWN,
  A_LEFT,
  A_RIGHT,
  A_DIG_LEFT,
  A_DIG_RIGHT,
  A_ENTER
} action_t;

// -----------------------------------------------------------------------------
// Global data

typedef struct {
  anim_def_t *idle;
  anim_def_t *walk;
  anim_def_t *fall;
  anim_def_t *climb;
  anim_def_t *hang;
  anim_def_t *dig;
  anim_def_t *die;
  anim_def_t *shout;
  anim_def_t *respawn;
  anim_def_t *dot;
} animations_t;

typedef struct {
  font_t *font;
  noise_t *noise;

  sound_t music;
  sound_t music_intro;
  sound_t music_outro;

  entity_t *player;
  int dump_debug;

  float level_time;
  int level;

  int level_golds;
  vec2_t exit_ladders[128];
  int exit_ladder_count;
  bool level_complete;

  vec2_t spawn_pos[32];
  int guard_count;
} global_t;

extern global_t g;

typedef struct {
  int level_pack;
  int max_levels[32];
  int level;
} options_t;

extern options_t opts;

// -----------------------------------------------------------------------------
// Scenes

extern scene_t scene_preload;
extern scene_t scene_game;
extern scene_t scene_menu;
extern scene_t scene_level_start;
extern scene_t scene_level_complete;
extern scene_t scene_game_over;

extern const char *mapChars;

void init_font();
void draw_text(char *text, int x, int y, int align);

bool load_options(options_t *o, const char *filename);
void save_options(options_t *o, const char *filename);

#endif