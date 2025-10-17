#include <math.h>

#include "entity.h"
#include "window.h"

EntityGlobals entity_globals;

const size_t ENTITY_COUNT = 1000;
Entity entities[ENTITY_COUNT];
size_t available[ENTITY_COUNT] = {};
size_t available_idx = ENTITY_COUNT - 1;

void init_entities() {
  entity_globals.pixart_unit = round((float) window.width / (float) window.height * 1.6279);
  for (size_t i=0; i < ENTITY_COUNT; i++) {
    available[i] = ENTITY_COUNT - i - 1;
  }
}

void visit_entities(void (*visitor)(Entity*, void*), void* data) {
  for (size_t i=0; i < ENTITY_COUNT; i++) {
    if (!entities[i].live) {
      continue;
    }
    visitor(&entities[i], data);
  }
}

float window_to_entity(float n) {
  return n / entity_globals.pixart_unit;
}

float entity_to_window(float n) {
  return n * entity_globals.pixart_unit;
}

Entity* create_entity(Entity new_a) {
  if (available_idx == 0) {
    return NULL;
  }
  size_t i = available[available_idx--];
  Entity* slot = &entities[i];
  new_a.live = true;
  *slot = new_a;
  return slot;
}
