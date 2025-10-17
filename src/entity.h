#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>

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
} Entity;

Entity* create_entity(Entity new_entity);

extern EntityGlobals entity_globals;

void init_entities();
float window_to_entity(float x);
float entity_to_window(float x);
void visit_entities(void (*visitor)(Entity*, void*), void* data);

#endif
