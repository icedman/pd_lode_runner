#ifndef _RENDER_CUSTOM_H_
#define _RENDER_CUSTOM_H_

#define WINDOW_TITLE "Spelunky"
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 240

#define GAME_VENDOR "bitesize"
#define GAME_NAME "Spelunky"

#define SZ 1
#define RENDER_WIDTH (WINDOW_WIDTH / SZ)
#define RENDER_HEIGHT (WINDOW_HEIGHT / SZ)
#define RENDER_RESIZE_MODE RENDER_RESIZE_WIDTH
#define RENDER_SCALE_MODE RENDER_SCALE_DISCRETE

#define ENTITIES_MAX 1024
#define ENTITY_MAX_SIZE 64
#define ENTITY_MIN_BOUNCE_VELOCITY 10

// #define IMAGE_MAX_SOURCES 128

#define RENDER_ATLAS_SIZE 64
#define RENDER_ATLAS_GRID 8
#define RENDER_ATLAS_BORDER 0
#define RENDER_BUFFER_CAPACITY 2048
// #define RENDER_TEXTURES_MAX 128

#define ENGINE_MAX_TICK 0.1

#endif // _RENDER_CUSTOM_H_