#include <math.h>
#include <box2d/box2d.h>

#include "entity.h"
#include "window.h"
#include "graphics.h"

EntityGlobals entity_globals;

#define FREELIST_DATATYPE Entity
#include "freelist.h"

b2WorldId world;

void init_entities() {
  Entity_FreelistInit(1000);
  update_pixart_unit();
  b2WorldDef worldDef = b2DefaultWorldDef();
  worldDef.gravity = (b2Vec2){0.0f, 100.0f};
  world = b2CreateWorld(&worldDef);
}

void update_pixart_unit() {
  entity_globals.pixart_unit = fmax(1, round(sqrt(window.width * window.height) / 548.63467));
}

void visit_entities(void (*visitor)(Entity*, void*), void* data) {
  for (size_t i=0; i < Entity_FreelistSize; i++) {
    if (Entity_FreelistHeap[i].free) {
      continue;
    }
    visitor(&Entity_FreelistHeap[i].item, data);
  }
}

float window_to_entity(float n) {
  return n / entity_globals.pixart_unit;
}

float entity_to_screen(float n) {
  return n * entity_globals.pixart_unit;
}

Entity* create_entity(Entity new_entity) {
  Entity* entity = Entity_FreelistAlloc();
  *entity = new_entity;
  entity->enabled = true;
  return entity;
}

void destroy_entity(Entity* entity) {
  Entity_FreelistFree(entity);
}

void attach_body(Entity* entity, bool dynamic) {
  if (B2_IS_NON_NULL(entity->body)) {
    return;
  }

  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = dynamic ? b2_dynamicBody : b2_staticBody;
  bodyDef.position = (b2Vec2){entity->x+0.1, entity->y};
  // bodyDef.motionLocks = (b2MotionLocks){ .angularZ = true };
  entity->body = b2CreateBody(world, &bodyDef);

  if (entity->poly.points) {
    size_t poly_count;
    Polygon* convexes = partition_convex(&entity->poly, &poly_count);
    for (size_t i=0; i < poly_count; i++) {
      b2ShapeDef shapeDef = b2DefaultShapeDef();
      shapeDef.density = 1.0f;
      shapeDef.material.friction = 1;
      shapeDef.material.restitution = 0;
      b2Hull hull = b2ComputeHull((b2Vec2*)convexes[i].points, convexes[i].count);
      if (!b2ValidateHull(&hull)) {
        printf("failed to validate hull\n");
        continue;
      }
      b2Polygon box = b2MakePolygon(&hull, 1);
      b2CreatePolygonShape(entity->body, &shapeDef, &box);
    }
  } else {
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 1;
    shapeDef.material.restitution = 0;
    b2Polygon box = b2MakeBox(entity->w/2, entity->h/2);
    b2CreatePolygonShape(entity->body, &shapeDef, &box);
  }
}

static void render_entity(Entity* entity, void* _data) {
  if (!entity->enabled) {
    return;
  }
  if (entity->poly.points != NULL) {
    draw_polygon(&entity->poly);
    return;
  }
  draw_quad((QuadData){
    .x = entity->x,
    .y = entity->y,
    .w = entity->w,
    .h = entity->h,
    .angle = entity->angle,
    .r = entity->color.r,
    .g = entity->color.g,
    .b = entity->color.b,
  });
}

void enable_entity(Entity* entity) {
  entity->enabled = true;
  if (B2_IS_NON_NULL(entity->body)) {
    b2Body_Disable(entity->body);
  }
}
void disable_entity(Entity* entity) {
  entity->enabled = false;
  if (B2_IS_NON_NULL(entity->body)) {
    b2Body_Enable(entity->body);
  }
}

void render_entities() {
  visit_entities(render_entity, NULL);
}

static void update_entity(Entity* entity, void* _data) {
  if (B2_IS_NULL(entity->body) || !b2Body_IsValid(entity->body)) {
    return;
  }

  b2Vec2 pos = b2Body_GetPosition(entity->body);
  entity->x = pos.x;
  entity->y = pos.y;
  entity->angle = b2Rot_GetAngle(b2Body_GetRotation(entity->body));
}

void update_entities() {
  b2World_Step(world, 1.0/60.0, 10);
  visit_entities(update_entity, NULL);
}
