#ifndef MAP_HINT_H
#define MAP_HINT_H

#include "game_main.h"

#define MAX_TILES 256

typedef struct map_tile_t {
  int tile_idx;
  int climb_id;
  int fall_id;
  int platform_id;
  int pit_id;
  vec2i_t pos;
  bool visited; // for a-star
} map_tile_t;

typedef struct map_path_t {
  int group_id;
  map_tile_t *tiles[MAX_TILES];
  int count;
  map_tile_t *platforms[MAX_TILES];
  int platform_count;
  map_tile_t *climbs[MAX_TILES];
  int climb_count;
  map_tile_t *falls[MAX_TILES];
  int fall_count;
} map_path_t;

typedef struct map_astar_path_t {
  map_tile_t *tiles[MAX_TILES];
  int count;
} map_astar_path_t;

map_tile_t *generate_map_hints(int w, int h);

void update_map_hints();
void find_path(entity_t *guard, entity_t *player);
map_tile_t *get_map_hint_tile(int tx, int ty);

map_tile_t *find_astar_path(entity_t *guard, entity_t *player);

map_tile_t *find_random_platform();

#endif // MAP_HINT_H