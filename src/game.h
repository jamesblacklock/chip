#ifndef GAME_H
#define GAME_H

#include <math.h>

#include "polygon.h"

typedef struct Map {
  Polygon* polys;
  size_t poly_count;
} Map;

bool map_save_file(const char* filename, Map* map);
Map map_load_file(const char* filename);
void map_free(Map* map);
void map_render(Map* map);

void update_pixart_unit();


typedef struct GameGlobals {
  float pixart_unit;
} GameGlobals;

extern GameGlobals game_globals;

static inline float window_to_pixart(float n) {
  return n / game_globals.pixart_unit;
}

static inline float pixart_to_window(float n) {
  return n * game_globals.pixart_unit;
}

#endif