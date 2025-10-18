#include <math.h>

#include "entity.h"
#include "window.h"
#include "graphics.h"
#include <chipmunk/chipmunk.h>

cpSpace* space;

EntityGlobals entity_globals;

const size_t ENTITY_COUNT = 1000;
Entity entities[ENTITY_COUNT];
size_t available[ENTITY_COUNT] = {};
size_t available_idx = ENTITY_COUNT - 1;

void init_entities() {
  entity_globals.pixart_unit = round(sqrt(window.width * window.height) / 548.63467);
  for (size_t i=0; i < ENTITY_COUNT; i++) {
    available[i] = ENTITY_COUNT - i - 1;
  }

  space = cpSpaceNew();
	cpSpaceSetIterations(space, 30);
	cpSpaceSetGravity(space, cpv(0, 300));
	cpSpaceSetSleepTimeThreshold(space, 0.5f);
	cpSpaceSetCollisionSlop(space, 0.1f);
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

void attach_body(Entity* entity, bool dynamic) {
  if (entity->body) {
    return;
  }

  float mass = 1;
  entity->body = cpSpaceAddBody(space, cpBodyNew(mass, INFINITY));//fmax(cpMomentForBox(mass, entity->w, entity->h), 0.001)));
  cpBodySetType(entity->body, dynamic ? CP_BODY_TYPE_DYNAMIC : CP_BODY_TYPE_STATIC);
  entity->shape = cpSpaceAddShape(space, cpBoxShapeNew(entity->body, entity->w, entity->h, 0.0));
  cpBodySetPosition(entity->body, cpv(entity->x, entity->y));
  cpShapeSetElasticity(entity->shape, 0);
  cpShapeSetFriction(entity->shape, 1);
  // cpBodySetAngle(entity->body, M_PI/4);
  cpSpaceReindexShapesForBody(space, entity->body);
}

void render_entity(Entity* entity, void* _data) {
  float pos[2];
  float sz[2];
  screen_to_world(entity_to_window(entity->x), entity_to_window(entity->y), pos);
  screen_to_world(entity_to_window(entity->w), entity_to_window(entity->h), sz);
  draw_quad((QuadData){
    .x = pos[0],
    .y = pos[1],
    .w = sz[0],
    .h = sz[1],
    .a = entity->angle,
    .r = entity->color.r,
    .g = entity->color.g,
    .b = entity->color.b,
  });
}

void render_entities() {
  visit_entities(render_entity, NULL);
}

void update_entity(Entity* entity, void* _data) {
  if (!entity->body) {
    return;
  }
  cpVect pos = cpBodyGetPosition(entity->body);
  entity->x = pos.x;
  entity->y = pos.y;
  entity->angle = cpBodyGetAngle(entity->body);
}

void update_entities() {
  visit_entities(update_entity, NULL);
}
