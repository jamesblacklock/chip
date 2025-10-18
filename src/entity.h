#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include <box2d/box2d.h>

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
  struct { float x; float y; float a; } velocity;
  float angle;
  Color color;
  bool live;

  b2BodyId body;
} Entity;

Entity* create_entity(Entity new_entity);
void attach_body(Entity* entity, bool dynamic);

extern EntityGlobals entity_globals;
extern b2WorldId world;

void init_entities();
float window_to_entity(float x);
float entity_to_window(float x);
void visit_entities(void (*visitor)(Entity*, void*), void* data);
void render_entities();
void update_entities();

#endif
