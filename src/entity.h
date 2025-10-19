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

typedef struct Entity {
  float x;
  float y;
  float w;
  float h;
  Polygon poly;
  struct { float x; float y; float a; } velocity;
  float angle;
  Color color;
  size_t index;
  b2BodyId body;
  bool enabled;
} Entity;

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C 
#endif

extern EntityGlobals entity_globals;
extern b2WorldId world;

EXTERN_C Entity* create_entity(Entity new_entity);
EXTERN_C void enable_entity(Entity* entity);
EXTERN_C void disable_entity(Entity* entity);
EXTERN_C void destroy_entity(Entity* entity);
EXTERN_C void attach_body(Entity* entity, bool dynamic);

EXTERN_C void init_entities();
EXTERN_C void update_pixart_unit();
EXTERN_C float window_to_entity(float x);
EXTERN_C float entity_to_screen(float x);
EXTERN_C void visit_entities(void (*visitor)(Entity*, void*), void* data);
EXTERN_C void render_entities();
EXTERN_C void update_entities();

#endif
