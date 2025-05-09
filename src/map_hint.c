#include "map_hint.h"

static int platform_id = 0;
static int climb_id = 0;
static int fall_id = 0;

extern const char *mapChars;

map_path_t platform_paths[MAX_TILES];
map_path_t climb_paths[MAX_TILES];
map_path_t fall_paths[MAX_TILES];
map_tile_t *hint_tiles;

map_tile_t *generate_map_hints(int w, int h) {
  int map_area = w * h;
  map_tile_t *tiles = (map_tile_t *)malloc(map_area * sizeof(map_tile_t));
  memset(tiles, 0, map_area * sizeof(map_tile_t));
  platform_id = 0;
  climb_id = 0;
  fall_id = 0;

  hint_tiles = tiles;
  update_map_hints();
  return tiles;
}

map_tile_t *find_random_platform() {
  map_tile_t *res = NULL;
  map_tile_t *fall_back = NULL;
  for (int i = 0; i < 64; i++) {
    map_path_t *p = &platform_paths[rand_int(0, platform_id - 1)];
    if (p->count > 0) {
      map_tile_t *t = p->tiles[rand_int(0, p->count - 1)];
      if (!fall_back) {
        fall_back = t;
      }
      if (t->climb_id == 0 && t->fall_id == 0) {
        res = t;
        break;
      }
    }
  }
  if (!res) {
    res = fall_back;
  }

  if (!res) {
    printf("oops!\n");
  }
  return res;
}

void update_map_hints() {
  map_tile_t *tiles = hint_tiles;
  map_t *col = engine.collision_map;
  map_t *bg = engine.background_maps[0];

  int w = bg->size.x;
  int h = bg->size.y;

  memset(platform_paths, 0, sizeof(map_path_t) * MAX_TILES);
  memset(climb_paths, 0, sizeof(map_path_t) * MAX_TILES);
  memset(fall_paths, 0, sizeof(map_path_t) * MAX_TILES);

  // find platforms
  platform_id = 1;
  for (int y = 0; y < h; y++) {
    bool prev_traverse = false;
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      int c = map_tile_at(col, vec2i(x, y));
      int b = map_tile_at(bg, vec2i(x, y));
      int ch = mapChars[b];
      int c_below = map_tile_at(col, vec2i(x, y + 1));
      int b_below = map_tile_at(bg, vec2i(x, y + 1));
      int ch_below = mapChars[b_below];
      bool can_traverse =
          (c == 0 && (c_below || ch == 'H' || ch == '-' || ch_below == 'H'));
      if (can_traverse) {
        tiles[idx].tile_idx = ch;
        tiles[idx].platform_id = platform_id;
        tiles[idx].pos = vec2i(x, y);
        platform_paths[platform_id].group_id = platform_id;
        platform_paths[platform_id].tiles[platform_paths[platform_id].count++] =
            &tiles[idx];
      } else {
        if (prev_traverse) {
          platform_id++;
        }
      }
      prev_traverse = can_traverse;
    }
  }

  printf("plaforms: %d\n", platform_id);

#if 1
  // dump platform
  printf("platform\n");
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      map_tile_t *tile = &tiles[idx];
      if (tile->platform_id == 0) {
        printf("   ");
      } else {
        printf("%02d ", tile->platform_id);
      }
    }
    printf("\n");
  }
  printf("-------------\n");
#endif

  // find climb
  climb_id = 1;
  for (int x = 0; x < w; x++) {
    bool prev_traverse = false;
    for (int y = 0; y < h; y++) {
      int idx = y * w + x;
      int c = map_tile_at(col, vec2i(x, y));
      int b = map_tile_at(bg, vec2i(x, y));
      int ch = mapChars[b];
      int c_below = map_tile_at(col, vec2i(x, y + 1));
      int b_below = map_tile_at(bg, vec2i(x, y + 1));
      int ch_below = mapChars[b_below];
      bool can_traverse = (c == 0 && (ch == 'H' || ch_below == 'H'));
      if (can_traverse) {
        tiles[idx].climb_id = climb_id;
        tiles[idx].pos = vec2i(x, y);
        climb_paths[climb_id].group_id = climb_id;
        climb_paths[climb_id].tiles[climb_paths[climb_id].count++] =
            &tiles[idx];
      } else {
        if (prev_traverse) {
          climb_id++;
        }
      }
      prev_traverse = can_traverse;
    }
  }

  printf("climbs: %d\n", climb_id);

#if 0
  // dump climb
  printf("climb\n");
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      map_tile_t *tile = &tiles[idx];
      if (tile->climb_id == 0) {
        printf("   ");
      } else {
        printf("%02d ", tile->climb_id);
      }
    }
    printf("\n");
  }
  printf("-------------\n");
#endif

  // find fall
  fall_id = 1;
  for (int x = 0; x < w; x++) {
    bool prev_traverse = false;
    bool in_fall = false;
    for (int y = 0; y < h; y++) {
      int idx = y * w + x;
      int c = map_tile_at(col, vec2i(x, y));
      int b = map_tile_at(bg, vec2i(x, y));
      int ch = mapChars[b];
      int c_below = map_tile_at(col, vec2i(x, y + 1));
      int b_below = map_tile_at(bg, vec2i(x, y + 1));
      int ch_below = mapChars[b_below];
      if (!in_fall) {
        bool can_fall = (ch == '-' || ch == 'H' || ch_below == 'H');
        if (!can_fall) {
          // check ledge
          if (c == 0 && c_below == 0) {
            if (x > 1 && map_tile_at(col, vec2i(x - 1, y + 1)) != 0) {
              can_fall = true;
            } else if (x + 2 < w &&
                       map_tile_at(col, vec2i(x + 1, y + 1)) != 0) {
              can_fall = true;
            }
          }
        }
        if (can_fall) {
          in_fall = true;
          prev_traverse = false;
        }
        if (!in_fall) {
          continue;
        }
      }
      bool can_traverse = c == 0;
      if (can_traverse) {
        tiles[idx].fall_id = fall_id;
        tiles[idx].pos = vec2i(x, y);
        fall_paths[fall_id].group_id = fall_id;
        fall_paths[fall_id].tiles[fall_paths[fall_id].count++] = &tiles[idx];
      } else {
        in_fall = false;
        if (prev_traverse) {
          fall_id++;
        }
      }
      prev_traverse = can_traverse;
    }
  }

  printf("falls: %d\n", fall_id);

#if 1
  // dump fall
  printf("fall\n");
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      map_tile_t *tile = &tiles[idx];
      if (tile->fall_id == 0) {
        printf("   ");
      } else {
        printf("%02d ", tile->fall_id);
      }
    }
    printf("\n");
  }
  printf("-------------\n");
#endif

  //--------------------
  // compute links
  //--------------------
  // platform
  for (int i = 0; i < platform_id; i++) {
    map_path_t *path = &platform_paths[i];
    if (path->count == 0)
      continue;

    int idx = path->tiles[0]->pos.y * w + path->tiles[0]->pos.x;

    // ledge
    if (path->tiles[0]->pos.x > 1) {
      map_tile_t *t = &tiles[idx - 1];
      if (t->fall_id) {
        map_path_t *f = &fall_paths[t->fall_id];
        if (f->count > 1) {
          path->falls[path->fall_count++] = t;
        }
      }
    }
    if (path->tiles[0]->pos.x + 2 < w) {
      map_tile_t *t = &tiles[idx + 1];
      if (t->fall_id) {
        map_path_t *f = &fall_paths[t->fall_id];
        if (f->count > 1) {
          path->falls[path->fall_count++] = t;
        }
      }
    }

    for (int j = 0; j < path->count; j++) {
      map_tile_t *tile = path->tiles[j];

      // climb
      if (tile->climb_id != 0) {
        path->climbs[path->climb_count++] = tile;
      }

      // fall
      if (tile->fall_id != 0) {
        path->falls[path->fall_count++] = tile;
      }
    }

    // link both edges of platform to fall path
    if (path->count > 0 && path->group_id == 1) {
      // left side
      {
        int x = path->tiles[0]->pos.x - 1;
        int y = path->tiles[0]->pos.y;
        if (x > 1) {
          int idx = y * w + (x - 1);
          map_tile_t *fall_tile = &tiles[idx];
          if (fall_tile->fall_id != 0) {
            path->falls[path->fall_count++] = fall_tile;
          }
        }
      }

      // right side
      {
        int x = path->tiles[path->count - 1]->pos.x;
        int y = path->tiles[path->count - 1]->pos.y;
        if (x > 0) { // width
          int idx = y * w + (x + 1);
          map_tile_t *fall_tile = &tiles[idx];
          if (fall_tile->fall_id != 0) {
            path->falls[path->fall_count++] = fall_tile;
          }
        }
      }
    }
  }

  // climb
  for (int i = 0; i < climb_id; i++) {
    map_path_t *path = &climb_paths[i];
    for (int j = 0; j < path->count; j++) {
      map_tile_t *tile = path->tiles[j];

      // platform
      if (tile->platform_id != 0) {
        path->platforms[path->platform_count++] = tile;
      }
    }
  }

  // fall
  for (int i = 0; i < fall_id; i++) {
    map_path_t *path = &fall_paths[i];
    for (int j = 0; j < path->count; j++) {
      map_tile_t *tile = path->tiles[j];

      // platform
      if (path->group_id == 20) {
        printf(">>%d (%d %d)\n", tile->platform_id, tile->pos.x, tile->pos.y);
      }

      if (tile->platform_id != 0) {
        path->platforms[path->platform_count++] = tile;
      }
    }
  }
}

static inline bool leads_up_to_target(map_path_t *target_platform_path,
                                      map_tile_t *to) {
  int group_id = to->climb_id;
  for (int o = 0; o < target_platform_path->climb_count; o++) {
    map_tile_t *t = target_platform_path->climbs[o];
    if (t->climb_id == group_id) {
      return true;
    }
  }
  return false;
}

static inline bool leads_down_to_target(map_path_t *target_platform_path,
                                        map_tile_t *to) {
  int group_id = to->fall_id;
  for (int o = 0; o < target_platform_path->fall_count; o++) {
    map_tile_t *t = target_platform_path->falls[o];
    if (t->fall_id == group_id) {
      return true;
    }
  }
  return false;
}

static inline map_tile_t *scanDown(map_tile_t *from, map_tile_t *target) {
  map_path_t *fall_path = &fall_paths[from->fall_id];
  // check this fall path for linked platforms
  for (int i = 0; i < fall_path->platform_count; i++) {
    map_tile_t *platform_tile = fall_path->platforms[i];
    if (platform_tile->pos.y <= from->pos.y) {
      continue;
    }
    if (platform_tile->platform_id == target->platform_id) {
      return platform_tile;
    }
  }
  return NULL;
}

static inline map_tile_t *scanUp(map_tile_t *from, map_tile_t *target) {
  map_path_t *climb_path = &climb_paths[from->climb_id];
  // check this climb path for linked platforms
  for (int i = 0; i < climb_path->platform_count; i++) {
    map_tile_t *platform_tile = climb_path->platforms[i];
    if (platform_tile->pos.y >= from->pos.y) {
      continue;
    }
    if (platform_tile->platform_id == target->platform_id) {
      return platform_tile;
    }
  }
  return NULL;
}

static inline map_tile_t *scanDownNearest(map_tile_t *from,
                                          map_tile_t *target) {
  map_path_t *fall_path = &fall_paths[from->fall_id];

  map_tile_t *nearest = NULL;
  float distance = -1;

  for (int i = 0; i < fall_path->platform_count; i++) {
    map_tile_t *platform_tile = fall_path->platforms[i];
    if (platform_tile->pos.y <= from->pos.y) {
      continue;
    }
    float dist = abs(platform_tile->pos.y - target->pos.y);
    if (dist < distance || nearest == NULL) {
      distance = dist;
      nearest = platform_tile;
      // so that platforms above are preferred
      if (platform_tile->pos.y > target->pos.y) {
        distance += 4;
      }
    }
  }
  return nearest;
}

static inline map_tile_t *scanUpNearest(map_tile_t *from, map_tile_t *target) {
  map_path_t *climb_path = &climb_paths[from->climb_id];

  map_tile_t *nearest = NULL;
  float distance = -1;

  // check this climb path for linked platforms
  for (int i = 0; i < climb_path->platform_count; i++) {
    map_tile_t *platform_tile = climb_path->platforms[i];
    if (platform_tile->pos.y >= from->pos.y) {
      continue;
    }
    float dist = abs(platform_tile->pos.y - target->pos.y);
    if (dist < distance || nearest == NULL) {
      distance = dist;
      nearest = platform_tile;
      // so that platforms above are preferred
      if (platform_tile->pos.y > target->pos.y) {
        distance += 4;
      }
    }
  }
  return nearest;
}

static inline float
computer_waypoint_travel_distance(vec2i_t from, vec2i_t *waypoints, int count) {
  float dst = 0;
  for (int i = 0; i < count; i++) {
    vec2i_t d = vec2i_sub(waypoints[i], from);
    dst += vec2_len(vec2(d.x, d.y));
    from = waypoints[i];
  }
  return dst;
}

map_tile_t *get_map_hint_tile(int tx, int ty) {
  map_tile_t *tiles = hint_tiles;
  map_t *col = engine.collision_map;

  if (tx < 0 || ty < 0)
    return NULL;

  if (tx >= col->size.x || ty >= col->size.y)
    return NULL;

  int playerIdx = ty * col->size.x + tx;
  map_tile_t *playerTile = &tiles[playerIdx];
  return playerTile;
}

void find_path(entity_t *guard, entity_t *player) {
  map_tile_t *tiles = hint_tiles;
  map_t *col = engine.collision_map;
  map_t *bg = engine.background_maps[0];

  int sx = floor(guard->pos.x / 16);
  int sy = floor(guard->pos.y / 16);
  int tx = floor(player->pos.x / 16);
  int ty = floor(player->pos.y / 16);

  int guardIdx = sy * col->size.x + sx;
  int playerIdx = ty * col->size.x + tx;

  int maxIdx = col->size.x * col->size.y;

  // TODO... crashes! championship #3
  if (guardIdx < 0 || playerIdx < 0)
    return;

  map_tile_t *guardTile = &tiles[guardIdx];
  map_tile_t *playerTile = &tiles[playerIdx];

  // printf("??%f %f\n", player->player.pos.x, player->player.pos.y);
  if (guardTile->platform_id == 0) {
    return;
  }

  // find a valid platform for player if falling
  if (playerTile->platform_id == 0) {
    if (player->state.state == STATE_FALL) {
      for (int i = 0; i < 40; i++) {
        playerIdx = (ty + i) * col->size.x + tx;
        playerTile = &tiles[playerIdx];
        if (playerTile->platform_id != 0) {
          break;
        }
      }
    } else if (player->last_platform_tile) {
      // use player's last known location
      playerTile = player->last_platform_tile;
    }
  }

  if (guardTile->platform_id != 0) {
    guard->last_platform_id = guardTile->platform_id;
    guard->last_platform_tile = guardTile;
  }
  guard->last_fall_id = guardTile->fall_id;
  if (playerTile->platform_id != 0) {
    player->last_platform_id = playerTile->platform_id;
    player->last_platform_tile = playerTile;
  }
  player->last_fall_id = playerTile->fall_id;

  //------------
  // beeline
  //------------
  // on the same platform
  if (guardTile->platform_id == playerTile->platform_id &&
      playerTile->platform_id != 0) {
    guard->guard.waypoint_count = 0;
    guard->guard.waypoints[guard->guard.waypoint_count++] = playerTile->pos;
    return;
  }

  // on the same ladder
  if (guardTile->climb_id == playerTile->climb_id &&
      playerTile->climb_id != 0) {
    guard->guard.waypoint_count = 0;
    guard->guard.waypoints[guard->guard.waypoint_count++] = playerTile->pos;
    return;
  }

  guard->guard.waypoint_origin = guardTile->pos;
  guard->guard.waypoint_count = 0;

  map_path_t *guard_pp = &platform_paths[guardTile->platform_id];

  //------------
  // scanDown
  //------------
  {
    int hits = 0;
    float nearestHit = -1;
    for (int i = 0; i < guard_pp->fall_count; i++) {
      map_tile_t *fall_tile = guard_pp->falls[i];
      if (fall_tile->pos.y > playerTile->pos.y) {
        continue;
      }

      map_tile_t *hit = scanDown(fall_tile, playerTile);
      if (hit) {
        hits++;
        vec2i_t waypoints[MAX_WAYPOINTS];
        int waypoint_count = 0;
        waypoints[waypoint_count++] = fall_tile->pos;
        waypoints[waypoint_count++] = hit->pos;
        // check travel
        float distance = computer_waypoint_travel_distance(
            vec2i(sx, sy), waypoints, waypoint_count);
        if (distance < nearestHit || nearestHit == -1) {
          memcpy(&guard->guard.waypoints, &waypoints,
                 sizeof(vec2i_t) * MAX_WAYPOINTS);
          guard->guard.waypoint_count = waypoint_count;
          nearestHit = distance;
        }
      }
    }

    if (nearestHit != -1) {
      return;
    }
  }

  //------------
  // scanUp
  //------------
  {
    int hits = 0;
    float nearestHit = -1;
    for (int i = 0; i < guard_pp->climb_count; i++) {
      map_tile_t *climb_tile = guard_pp->climbs[i];
      if (climb_tile->pos.y < playerTile->pos.y) {
        continue;
      }

      map_tile_t *hit = scanUp(climb_tile, playerTile);
      if (hit) {
        hits++;
        vec2i_t waypoints[MAX_WAYPOINTS];
        int waypoint_count = 0;
        waypoints[waypoint_count++] = climb_tile->pos;
        waypoints[waypoint_count++] = hit->pos;
        // check travel
        float distance = computer_waypoint_travel_distance(
            vec2i(sx, sy), waypoints, waypoint_count);
        if (distance < nearestHit || nearestHit == -1) {
          memcpy(&guard->guard.waypoints, &waypoints,
                 sizeof(vec2i_t) * MAX_WAYPOINTS);
          guard->guard.waypoint_count = waypoint_count;
          nearestHit = distance;
        }
      }
    }

    if (nearestHit != -1) {
      return;
    }
  }

#ifndef ENABLE_ASTAR

  //------------
  // scanDown -- the nearest possible to target
  //------------
  {
    int hits = 0;
    float nearestHit = -1;
    for (int i = 0; i < guard_pp->fall_count; i++) {
      map_tile_t *fall_tile = guard_pp->falls[i];
      if (fall_tile->pos.y > playerTile->pos.y) {
        continue;
      }

      map_tile_t *hit = scanDownNearest(fall_tile, playerTile);
      if (hit) {
        hits++;
        vec2i_t waypoints[MAX_WAYPOINTS];
        int waypoint_count = 0;
        waypoints[waypoint_count++] = fall_tile->pos;
        waypoints[waypoint_count++] = hit->pos;
        // check travel
        float distance = computer_waypoint_travel_distance(
            vec2i(sx, sy), waypoints, waypoint_count);
        if (distance < nearestHit || nearestHit == -1) {
          memcpy(&guard->guard.waypoints, &waypoints,
                 sizeof(vec2i_t) * MAX_WAYPOINTS);
          guard->guard.waypoint_count = waypoint_count;
          nearestHit = distance;
        }
      }
    }

    if (nearestHit != -1) {
      return;
    }
  }

  //------------
  // scanUp -- the nearest possible to target
  //------------
  {
    int hits = 0;
    float nearestHit = -1;
    for (int i = 0; i < guard_pp->climb_count; i++) {
      map_tile_t *climb_tile = guard_pp->climbs[i];
      if (climb_tile->pos.y < playerTile->pos.y) {
        continue;
      }

      map_tile_t *hit = scanUpNearest(climb_tile, playerTile);
      if (hit) {
        hits++;
        vec2i_t waypoints[MAX_WAYPOINTS];
        int waypoint_count = 0;
        waypoints[waypoint_count++] = climb_tile->pos;
        waypoints[waypoint_count++] = hit->pos;
        // check travel
        float distance = computer_waypoint_travel_distance(
            vec2i(sx, sy), waypoints, waypoint_count);
        if (distance < nearestHit || nearestHit == -1) {
          memcpy(&guard->guard.waypoints, &waypoints,
                 sizeof(vec2i_t) * MAX_WAYPOINTS);
          guard->guard.waypoint_count = waypoint_count;
          nearestHit = distance;
        }
      }
    }

    if (nearestHit != -1) {
      return;
    }
  }

  // huh?
  if (guard->guard.waypoint_count == 0) {
    printf("%d %d - %d %d\n", sx, sy, tx, ty);
  }
#endif
}