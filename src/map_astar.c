#include "map_hint.h"

#define MAX_ASTAR_LOOP 256

extern map_tile_t *hint_tiles;
extern map_path_t platform_paths[MAX_TILES];
extern map_path_t climb_paths[MAX_TILES];
extern map_path_t fall_paths[MAX_TILES];

static map_astar_path_t astar_path;

static inline void add_to_list(map_astar_path_t *list, map_tile_t *tile) {
  list->tiles[list->count++] = tile;
}

static inline map_tile_t *top_of_list(map_astar_path_t *list) {
  if (list->count == 0) {
    return NULL;
  }
  return list->tiles[list->count - 1];
}

static inline map_tile_t *pop_from_list(map_astar_path_t *list) {
  if (list->count < 1) {
    return NULL;
  }
  list->count--;
  return list->tiles[list->count];
}

static inline bool is_in_list(map_astar_path_t *list, map_tile_t *tile) {
  for (int i = 0; i < list->count; i++) {
    return list->tiles[i] == tile;
  }
  return false;
}

// via links
static inline map_tile_t *find_neareset_neighbor_via_links(map_tile_t *tile,
                                                           map_tile_t *target) {
  map_tile_t *res = NULL;
  float distance = -1;

  vec2_t nearest_me = vec2(target->pos.x, target->pos.y);
  // vec2_t nearest_me = vec2(tile->pos.x, tile->pos.y);

  // check along the platform
  if (tile->platform_id != 0) {
    map_path_t *p = &platform_paths[tile->platform_id];
    if (p && p->climb_count) {
      for (int i = 0; i < p->climb_count; i++) {
        map_tile_t *n = p->climbs[i];
        if (!n->visited) {
          float dst = vec2_dist(nearest_me, vec2(n->pos.x, n->pos.y));
          if (dst < distance || distance == -1) {
            res = n;
            dst = distance;
          }
        }
      }
    }
    if (p && p->fall_count) {
      for (int i = 0; i < p->fall_count; i++) {
        map_tile_t *n = p->falls[i];
        if (n->pos.y < tile->pos.y)
          continue;
        if (!n->visited) {
          float dst = vec2_dist(nearest_me, vec2(n->pos.x, n->pos.y));
          if (dst < distance || distance == -1) {
            res = n;
            dst = distance;
          }
        }
      }
    }
  }

  // check along the ladder
  if (tile->climb_id != 0) {
    map_path_t *p = &climb_paths[tile->climb_id];
    if (p && p->platform_count) {
      for (int i = 0; i < p->platform_count; i++) {
        map_tile_t *n = p->platforms[i];
        if (n == tile)
          continue;
        if (!n->visited) {
          float dst = vec2_dist(nearest_me, vec2(n->pos.x, n->pos.y));
          if (dst < distance || distance == -1) {
            res = n;
            dst = distance;
          }
        }
      }
    }
    if (p && p->fall_count) {
      for (int i = 0; i < p->fall_count; i++) {
        map_tile_t *n = p->falls[i];
        if (n->pos.y < tile->pos.y)
          continue;
        if (!n->visited) {
          float dst = vec2_dist(nearest_me, vec2(n->pos.x, n->pos.y));
          if (dst < distance || distance == -1) {
            res = n;
            dst = distance;
          }
        }
      }
    }
  }

  // check along the fall path
  if (tile->fall_id != 0) {
    map_path_t *p = &fall_paths[tile->fall_id];
    if (p && p->platform_count) {
      for (int i = 0; i < p->platform_count; i++) {
        map_tile_t *n = p->platforms[i];
        if (n->pos.y < tile->pos.y)
          continue;
        if (n == tile)
          continue;
        if (!n->visited) {
          float dst = vec2_dist(nearest_me, vec2(n->pos.x, n->pos.y));
          if (dst < distance || distance == -1) {
            res = n;
            dst = distance;
          }
        }
      }
    }
  }
  return res;
}

static inline map_tile_t *find_neareset_neighbor(map_tile_t *tile,
                                                 map_tile_t *target) {
  map_tile_t *up = get_map_hint_tile(tile->pos.x, tile->pos.y - 1);
  if (up && up->climb_id > 0 && up->climb_id == tile->climb_id &&
      !up->visited) {
    // return up;
  } else {
    up = NULL;
  }

  map_tile_t *down = get_map_hint_tile(tile->pos.x, tile->pos.y + 1);
  if (down && down->fall_id > 0 && down->fall_id == tile->fall_id &&
      !down->visited) {
    // return down;
  } else {
    down = NULL;
  }

  map_tile_t *left = get_map_hint_tile(tile->pos.x - 1, tile->pos.y);
  if (left && left->platform_id > 0 && left->platform_id == tile->platform_id &&
      !left->visited) {
    // return left;
  } else {
    left = NULL;
  }

  map_tile_t *right = get_map_hint_tile(tile->pos.x + 1, tile->pos.y);
  if (right && right->platform_id > 0 &&
      right->platform_id != tile->platform_id && !right->visited) {
    // return right;
  } else {
    right = NULL;
  }

  map_tile_t *res = NULL;
  float res_dst = 0;
  map_tile_t *opts[] = {up, down, left, right};
  for (int i = 0; i < 4; i++) {
    map_tile_t *t = opts[i];
    if (!t)
      continue;
    float dst =
        vec2_dist(vec2(target->pos.x, target->pos.y), vec2(t->pos.x, t->pos.y));
    if (!res || dst < res_dst) {
      res = t;
      res_dst = dst;
    }
  }

  if (res) {
    return res;
  }

  return find_neareset_neighbor_via_links(tile, target);
}

map_tile_t *find_astar_path(entity_t *guard, entity_t *player) {
  map_tile_t *tiles = hint_tiles;

  map_t *col = engine.collision_map;
  int w = col->size.x;
  int h = col->size.y;

  map_astar_path_t open_list;
  open_list.count = 0;

  int sx = floor(guard->pos.x / 16);
  int sy = floor(guard->pos.y / 16);
  int tx = floor(player->pos.x / 16);
  int ty = floor(player->pos.y / 16);

  int src_idx = sy * w + sx;
  int dst_idx = ty * w + tx;

  for (int i = 0; i < (w * h); i++) {
    tiles[i].visited = false;
  }

  map_tile_t *src = &tiles[src_idx];
  map_tile_t *dst = &tiles[dst_idx];

  if (!src || !dst)
    return NULL;

  // // add platform tiles ...
  add_to_list(&open_list, src);
  src->visited = true;

  bool found = false;

  int i = 0;
  while (open_list.count) {
    map_tile_t *t = top_of_list(&open_list);

    if ((t->platform_id != 0 && t->platform_id == dst->platform_id) ||
        (t->climb_id != 0 && t->climb_id == dst->climb_id) ||
        (t->fall_id != 0 && t->fall_id == dst->fall_id)) {
      found = true;
      break;
    }

    if (open_list.count > 2) {
      float dst = vec2_dist(vec2(t->pos.x, t->pos.y), vec2(tx, ty));
      if (dst < 8) {
        found = true;
        break;
      }
    }

    map_tile_t *n = find_neareset_neighbor(t, dst);
    if (n) {
      add_to_list(&open_list, n);
      n->visited = true;
    } else {
      // add_to_list(&open_list, n);
      pop_from_list(&open_list);
    }

    if (open_list.count >= MAX_TILES) {
      printf("bail (MAX_TILES) %d\n", open_list.count);
      break;
    }
    if (i++ >= MAX_ASTAR_LOOP) {
      printf("bail (MAX_ASTAR_LOOP) %d\n", i);
      break;
    }
  }

  if (found || open_list.count > 10) {
    guard->guard.waypoint_origin = src->pos;
    guard->guard.waypoint_count = 0;
    for (int i = 0; i < open_list.count && i < MAX_WAYPOINTS - 1; i++) {
      guard->guard.waypoints[guard->guard.waypoint_count++] =
          open_list.tiles[i]->pos;
    }
    if (found) {
      guard->guard.waypoints[guard->guard.waypoint_count++] = dst->pos;
    }
    // printf("%d\n", guard->guard.waypoint_count);
    return open_list.tiles[0];
  }

  return NULL;
}