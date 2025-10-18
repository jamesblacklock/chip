#include <math.h>
#include <box2d/box2d.h>

#include "entity.h"
#include "window.h"
#include "graphics.h"

EntityGlobals entity_globals;

const size_t ENTITY_COUNT = 1000;
Entity entities[ENTITY_COUNT];
size_t available[ENTITY_COUNT] = {};
size_t available_idx = ENTITY_COUNT - 1;

b2WorldId world;

void init_entities() {
  entity_globals.pixart_unit = fmax(1, round(sqrt(window.width * window.height) / 548.63467));
  for (size_t i=0; i < ENTITY_COUNT; i++) {
    available[i] = ENTITY_COUNT - i - 1;
  }

  b2WorldDef worldDef = b2DefaultWorldDef();
  worldDef.gravity = (b2Vec2){0.0f, 100.0f};
  world = b2CreateWorld(&worldDef);
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
  if (B2_IS_NON_NULL(entity->body)) {
    return;
  }

  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = dynamic ? b2_dynamicBody : b2_staticBody;
  bodyDef.position = (b2Vec2){entity->x+0.1, entity->y};
  entity->body = b2CreateBody(world, &bodyDef);
  b2Polygon box = b2MakeBox(entity->w/2, entity->h/2);
  b2ShapeDef shapeDef = b2DefaultShapeDef();
  shapeDef.density = 1.0f;
  shapeDef.material.friction = 0.3f;
  b2CreatePolygonShape(entity->body, &shapeDef, &box);
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
  if (B2_IS_NULL(entity->body)) {
    return;
  }

  b2Vec2 pos = b2Body_GetPosition(entity->body);
  entity->x = pos.x;
  entity->y = pos.y;
  entity->angle = b2Rot_GetAngle(b2Body_GetRotation(entity->body));
}

void update_entities() {
  visit_entities(update_entity, NULL);
}
