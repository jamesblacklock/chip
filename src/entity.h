#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include <stddef.h>
#include <box2d/box2d.h>

#include "polygon.h"

typedef struct EntityGlobals {
  float pixart_unit;
} EntityGlobals;

typedef struct Color {
  float r;
  float g;
  float b;
} Color;

typedef struct Collision {
  PolygonIntersection pi;
} Collision;

typedef struct Entity {
  // float x;
  // float y;
  // float w;
  // float h;
  Polygon poly;
  struct { float x; float y; float a; } velocity;
  float angle;
  Color color;
  size_t index;
  bool dynamic;
  bool enabled;
  Collision collision;
} Entity;

typedef struct Map {
  Entity** polygons;
  size_t polygon_count;
} Map;

#define MAP_MAX_ENTITIES 6000

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C 
#endif

extern EntityGlobals entity_globals;

EXTERN_C Entity* create_entity(Entity new_entity);
EXTERN_C void enable_entity(Entity* entity);
EXTERN_C void disable_entity(Entity* entity);
EXTERN_C void free_entity(Entity* entity);
EXTERN_C void attach_body(Entity* entity, bool dynamic);

EXTERN_C void init_entities();
EXTERN_C void update_pixart_unit();
EXTERN_C float screen_to_entity(float x);
EXTERN_C float entity_to_screen(float x);
EXTERN_C void visit_entities(bool (*visitor)(Entity*, void*), void* data);
EXTERN_C void render_entities();
EXTERN_C void update_entities(float ms);
EXTERN_C void move_entity(Entity* entity, float x, float y);
EXTERN_C void snap_entity_to_surface(Entity* subject, Entity* target, size_t subject_point, size_t target_edge);

EXTERN_C bool save_map_file(const char* filename, Map* map);
EXTERN_C Map load_map_file(const char* filename);
EXTERN_C void free_map(Map* map);

#endif
