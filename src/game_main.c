#include "game_main.h"
#include "map_hint.h"

#define SCREEN_WIDTH (400 / 2)
#define SCREEN_HEIGHT (240 / 2)
#define CONFIG_FN "config.ini"

global_t g;
options_t opts;

entity_vtab_t entity_vtab_other = {};

static image_t *tileset;
static image_t *characters;
static camera_t camera;
static map_tile_t *map_hints = 0;

#define MAX_LEVEL_PACKS 5
const char *level_pack_paths[MAX_LEVEL_PACKS] = {
    "assets/levels/original.json",     "assets/levels/championship.json",
    "assets/levels/fanbook.json",      "assets/levels/revenge.json",
    "assets/levels/professional.json",
};
const char *level_pack_titles[MAX_LEVEL_PACKS] = {
    "original", "championship", "fanbook", "revenge", "professional",
};
int levels_per_pack[5] = {150, 51, 66, 17, 150};

const char *mapChars = " #@H-XS$0&. ?????????!";

/*
# brick
@ concrete
H ladder
- vertical pole
X trap door
S finishing ladder
$ gold
0 enemy
& runner
. pit
. pit with enemy
*/

static void preload_init(void) {
  load_options(&opts, CONFIG_FN);
  characters = image("assets/images/characters.qoi");
  tileset = image("assets/images/tileset.qoi");
  init_font();
}

static void preload_update(void) {
  // engine_set_scene(&scene_level_complete);
  engine_set_scene(&scene_menu);
  // engine_set_scene(&scene_game);
  // scene_base_update();
}

scene_t scene_preload = {.init = preload_init, .update = preload_update};

vec2_t random_spawn_pos() {
  int idx = rand_int(0, g.guard_count-1);
  vec2_t pos = g.spawn_pos[idx];
  printf("spawn #%d (%f %f)\n", idx, pos.x, pos.y);
  return pos;
}

bool flush_keys = false;
static void load_map(char *path, int level_id) {
  // reset game variables
  g.player = NULL;
  g.level_time = 0;
  g.level_golds = 0;
  g.exit_ladder_count = 0;
  g.level_complete = false;
  g.guard_count = 0;

  flush_keys = true;

  int guard_id = 0;

  json_t *json = platform_load_asset_json(path);
  for (int i = 0; i < json->len; i++) {
    json_t *level = json_value_at(json, i);

    if (i == level_id) {
      json_t *first = json_value_at(level, 0);
      int w = strlen(json_string(first));
      int h = level->len + 1;

      map_t *col = map_with_data(16, vec2i(w + 2, h + 1), NULL);
      engine_set_collision_map(col);
      map_t *map = map_with_data(16, vec2i(w + 2, h + 1), NULL);
      map->tileset = tileset;
      map->tileset = tileset;
      engine_add_background_map(map);

      if (map_hints) {
        free(map_hints);
      }
      map_hints = generate_map_hints(map->size.x, map->size.y);

      for (int j = 0; j < level->len; j++) {
        json_t *row = json_value_at(level, j);
        char *rows = json_string(row);
        int len = strlen(rows);
        int y = j + 1;

        // side walls
        {
          char *mc = strchr(mapChars, '@');
          int idx = mc - mapChars;
          map->data[(map->size.x * y)] = idx;
          col->data[(map->size.x * y)] = 1;
          map->data[(map->size.x * y) + (map->size.x - 1)] = idx;
          col->data[(map->size.x * y) + (map->size.x - 1)] = 1;
        }

        for (int k = 0; k < len; k++) {

          // bottom
          if (j == 0) {
            char *mc = strchr(mapChars, '@');
            int idx = mc - mapChars;
            map->data[(map->size.x * h) + k] = idx;
            col->data[(map->size.x * h) + k] = 1;
            // some over draw
            map->data[(map->size.x * h) + k + 1] = idx;
            col->data[(map->size.x * h) + k + 1] = 1;
            map->data[(map->size.x * h) + k + 2] = idx;
            col->data[(map->size.x * h) + k + 2] = 1;
          }

          int x = k + 1;
          char c = rows[k];
          int mapIdx = (map->size.x * y) + x;

          printf("%c", c);

          vec2_t pos = vec2(x * 16, y * 16);
          switch (c) {
          case ' ':
            continue;
          case '0': {
            // enemy
            pos.x += 4;
            pos.y += 2;
            entity_t *ent = entity_spawn(ENTITY_TYPE_GUARD, pos);
            ent->guard.guard_id = guard_id++;
            g.spawn_pos[g.guard_count++] = pos;
            continue;
          }
          case 'X': {
            // trap
            entity_t *ent = entity_spawn(ENTITY_TYPE_TRAP, pos);
            continue;
          }
          case '$': {
            // gold
            entity_t *ent = entity_spawn(ENTITY_TYPE_GOLD, pos);
            g.level_golds++;
            continue;
          }
          case '#':
          case '@':
            col->data[mapIdx] = 1;
            break;
          case '&': {
            // runner
            pos.x += 4;
            pos.y += 2;
            entity_t *ent = entity_spawn(ENTITY_TYPE_PLAYER, pos);
            g.player = ent;
            continue;
          }
          }

          if (c == 'S') {
            g.exit_ladders[g.exit_ladder_count++] = pos;
            continue;
          }

          char *mc = strchr(mapChars, c);
          if (mc) {
            int idx = mc - mapChars;
            map->data[mapIdx] = idx;
            // printf("%d\n", idx);
          }
        }
        printf("\n");
      }

      break;
    }
  }
  json_destroy(json);

  update_map_hints();
}

static void game_init(void) {
  sound_unpause(g.music);
  sound_pause(g.music_intro);

  engine.gravity = 200;

  camera.offset = vec2(4, -8);
  camera.speed = 3;
  camera.min_vel = 5;

  load_map(level_pack_paths[opts.level_pack], g.level);

  entity_list_t players = entities_by_type(ENTITY_TYPE_PLAYER);
  if (players.len > 0) {
    camera_follow(&camera, players.entities[0], true);
  }
}

void show_exit_ladder() {
  // show ladders
  map_t *bg = engine.background_maps[0];
  for (int i = 0; i < g.exit_ladder_count; i++) {
    map_set_tile_at_px(bg, g.exit_ladders[i], 6);
  }
  g.exit_ladder_count = 0;
}

static void game_update(void) {
  if (flush_keys) {
    if (input_state(A_DIG_LEFT) || input_state(A_DIG_RIGHT) ||
        input_state(A_LEFT) || input_state(A_RIGHT)) {
      return;
    }
    flush_keys = false;
  }

  scene_base_update();
  camera_update(&camera);

  g.level_time += engine.tick;

#ifdef TEST_MAP_LOADING
  g.level++;
  g.level = g.level % levels_per_pack[opts.level_pack];
  engine_set_scene(&scene_game);
#endif

#ifdef TEST_EXIT_LADDER
  show_exit_ladder();
#endif

  if (input_pressed(A_ENTER)) {
    if (input_state(A_DOWN)) {
      engine_set_scene(&scene_menu);
      return;
    }
    if (input_state(A_UP)) {
      g.level++;
      g.level = g.level % levels_per_pack[opts.level_pack];
      engine_set_scene(&scene_game);
      return;
    }
  }
}

static void game_draw(void) {
  render_clear(1);
  scene_base_draw();

  char tmp[64];
  sprintf(tmp, "Lv.%d $%d/%d", (g.level + 1), g.player->player.gold_collected,
          g.level_golds);
  render_push();
  // render_scale(vec2(0.5, 0.5));
  render_translate(vec2(4, 4));
  draw_text(tmp, 0, 0, 0);
  render_pop();
}

scene_t scene_game = {
    .init = game_init,
    .update = game_update,
    .draw = game_draw,
};

static void menu_init(void) {
  sound_pause(g.music_outro);
  printf("menu - press a key\n");
}

static void menu_update(void) {
  if (input_pressed(A_DIG_LEFT) || input_pressed(A_DIG_RIGHT)) {
    save_options(&opts, CONFIG_FN);
    engine_set_scene(&scene_level_start);
  }

  if (input_pressed(A_LEFT)) {
    opts.level_pack--;
    if (opts.level_pack < 0) {
      opts.level_pack = 0;
    }
  }
  if (input_pressed(A_RIGHT)) {
    opts.level_pack++;
    opts.level_pack = opts.level_pack % MAX_LEVEL_PACKS;
  }
}

static void menu_draw(void) {
  render_clear(1);
  scene_base_draw();

  char tmp[64];
  sprintf(tmp, "Lode Runner\nselect pack\n<%s>",
          level_pack_titles[opts.level_pack]);
  render_push();
  // render_scale(vec2(0.5, 0.5));
  render_translate(vec2(100, 60));
  draw_text(tmp, 0, 0, 0);
  render_pop();
}

scene_t scene_menu = {
    .init = menu_init, .update = menu_update, .draw = menu_draw};

static void level_start_init(void) {
  printf("start\n");
  sound_unpause(g.music_intro);
  sound_pause(g.music_outro);
  sound_pause(g.music);
}

static void level_start_update(void) {
  if (input_pressed(A_DIG_LEFT) || input_pressed(A_DIG_RIGHT)) {
    g.level = opts.level;
    save_options(&opts, CONFIG_FN);
    engine_set_scene(&scene_game);
  }
  if (input_pressed(A_LEFT)) {
    opts.level--;
    if (opts.level < 0) {
      opts.level = 0;
    }
  }
  if (input_pressed(A_RIGHT)) {
    opts.level++;
    opts.level = opts.level % levels_per_pack[opts.level_pack];
  }
}

static void level_start_draw(void) {
  render_clear(1);
  scene_base_draw();

  char tmp[64];
  sprintf(tmp, "Lode Runner\n%s\nselect level\n<%d>",
          level_pack_titles[opts.level_pack], (opts.level + 1));
  render_push();
  // render_scale(vec2(0.5, 0.5));
  render_translate(vec2(100, 60));
  draw_text(tmp, 4, 4, 0);
  render_pop();
}

scene_t scene_level_start = {.init = level_start_init,
                             .update = level_start_update,
                             .draw = level_start_draw};

void player_shout(entity_t *self);

static void level_complete_init(void) {
  printf("next level\n");
  g.level_complete = false;
  g.level++;
  g.level = g.level % levels_per_pack[opts.level_pack];
  opts.level = g.level;
  save_options(&opts, CONFIG_FN);

  sound_pause(g.music);
  sound_pause(g.music_intro);
  sound_unpause(g.music_outro);

  engine.gravity = 200;

  camera.offset = vec2(4, -8);
  camera.speed = 3;
  camera.min_vel = 5;

  // load_map(level_pack_paths[opts.level_pack], g.level);
  load_map("assets/levels/scenes.json", 0);

  entity_list_t players = entities_by_type(ENTITY_TYPE_PLAYER);
  if (players.len > 0) {
    entity_t *player = entity_by_ref(players.entities[0]);
    player->pos.x += 8;
    player_shout(player);
    camera_follow(&camera, players.entities[0], true);
  }
}

static void level_complete_update(void) {
  // engine_set_scene(&scene_level_start);
  if (input_pressed(A_DIG_LEFT) || input_pressed(A_DIG_RIGHT)) {
    engine_set_scene(&scene_level_start);
  }
}

static void level_complete_draw(void) {
  render_clear(1);
  scene_base_draw();

  render_push();
  // render_scale(vec2(0.5, 0.5));
  render_translate(vec2(100, 60));
  draw_text("level complete", 4, 4, 0);
  render_pop();
}

scene_t scene_level_complete = {.init = level_complete_init,
                                .update = level_complete_update,
                                .draw = level_complete_draw};

static void game_over_init(void) {}

static void game_over_update(void) { engine_set_scene(&scene_game); }

scene_t scene_game_over = {.init = game_over_init, .update = game_over_update};

void main_init(void) {
  // Gamepad
  input_bind(INPUT_GAMEPAD_DPAD_LEFT, A_LEFT);
  input_bind(INPUT_GAMEPAD_DPAD_RIGHT, A_RIGHT);
  input_bind(INPUT_GAMEPAD_L_STICK_LEFT, A_LEFT);
  input_bind(INPUT_GAMEPAD_L_STICK_RIGHT, A_RIGHT);
  input_bind(INPUT_GAMEPAD_B, A_DIG_LEFT);
  input_bind(INPUT_GAMEPAD_A, A_DIG_RIGHT);

  // Keyboard
  input_bind(INPUT_KEY_D, A_ENTER);
  input_bind(INPUT_KEY_LEFT, A_LEFT);
  input_bind(INPUT_KEY_RIGHT, A_RIGHT);
  input_bind(INPUT_KEY_UP, A_UP);
  input_bind(INPUT_KEY_DOWN, A_DOWN);
  input_bind(INPUT_KEY_X, A_DIG_LEFT);
  input_bind(INPUT_KEY_C, A_DIG_RIGHT);

  // g.noise = noise(8);
  // g.font = font("assets/font_04b03.qoi", "assets/font_04b03.json");
  g.music = sound(sound_source("assets/sfx/gameplay.qoa"));
  g.music_intro = sound(sound_source("assets/sfx/intro.qoa"));
  g.music_outro = sound(sound_source("assets/sfx/outro.qoa"));
  sound_set_loop(g.music_intro, false);
  sound_set_loop(g.music_outro, false);
  sound_set_loop(g.music, true);

  sound_set_global_volume(0.75);

  engine_set_scene(&scene_preload);
}

void main_cleanup(void) {}
