#include <math.h>
#include <box2d/box2d.h>

#include "main.h"
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
}

void update_pixart_unit() {
  entity_globals.pixart_unit = fmax(1, round(sqrt(window.width * window.height) / 548.63467));
}

void visit_entities(bool (*visitor)(Entity*, void*), void* data) {
  size_t count = 0;
  for (size_t i=1; i <= Entity_FreelistSize; i++) {
    size_t r = Entity_FreelistSize - i;
    if (Entity_FreelistHeap[r].free) {
      continue;
    }
    if (!visitor(&Entity_FreelistHeap[r].item, data)) {
      break;
    }
  }
}

float screen_to_entity(float n) {
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

void free_entity(Entity* entity) {
  Entity_FreelistFree(entity);
}

static bool render_entity(Entity* entity, void* _data) {
  if (!entity->enabled) {
    return true;
  }
  if (entity->poly.points != NULL) {
    draw_polygon(&entity->poly, entity->color.r, entity->color.g, entity->color.b);
  }
  return true;
}

void enable_entity(Entity* entity) {
  entity->enabled = true;
}
void disable_entity(Entity* entity) {
  entity->enabled = false;
}

void render_entities() {
  visit_entities(render_entity, NULL);
}

static bool update_entity(Entity* entity, void* _ms) {
  if (!entity->dynamic) {
    return true;
  }

  float ms = *(float*) _ms;
  float gravity = 15;
  if (entity->velocity.y < gravity) {
    entity->velocity.y = fmin(gravity, entity->velocity.y + ms / 100);
  }
  move_entity(entity, entity->velocity.x, entity->velocity.y);
  if (entity->collision.pi.intersects) {
    entity->velocity.x = 0;
    entity->velocity.y = 0;
  }
  return true;
}

void update_entities(float ms) {
  visit_entities(update_entity, &ms);
}

bool save_map_file(const char* filename, Map* map) {
  FILE* fp = fopen(filename, "wb");
  for (size_t i=0; i < map->polygon_count; i++) {
    write_object(&map->polygons[i]->poly, fp);
  }
  write_eof(fp);
  fclose(fp);
  return true;
}

Map load_map_file(const char* filename) {
  FILE* fp = fopen(filename, "rb");
  Polygon* poly = (Polygon*) read_object(fp);

  Entity** entities = malloc(sizeof(Entity*) * MAP_MAX_ENTITIES);
  size_t entity_count = 0;
  while (poly != NULL) {
    if (poly->__so.object_type != OBJECT_TYPE_POLYGON) {
      printf("encountered unexpected object: %d", poly->__so.object_type);
      continue;
    }
    if (validate_polygon(poly)) {
      Color color = { rand() / (float) RAND_MAX, rand() / (float) RAND_MAX, rand() / (float) RAND_MAX };
      entities[entity_count] = create_entity((Entity){ .poly = *poly, .color = color });
      entity_count++;
    }
    free(poly);
    poly = (Polygon*) read_object(fp);
  }
  fclose(fp);
  entities = realloc(entities, sizeof(Entity*) * entity_count);
  return (Map){ .polygons = entities, .polygon_count = entity_count };
}

void free_map(Map* map) {
  if (map->polygons != NULL) {
    for (size_t i=0; i < map->polygon_count; i++) {
      free_entity(map->polygons[i]);
    }
  }
}

typedef struct SubjectEntityInfo {
  float vx;
  float vy;
  Entity* subject;
} SubjectEntityInfo;

bool collide_entities(Entity* entity, void* _info) {
  SubjectEntityInfo* info = _info;
  if (!entity->poly.count) {
    printf("we need a polygon for collision!!\n");
    return true;
  }
  if (entity == info->subject) {
    return true;
  }

  Polygon* entity_poly = &entity->poly;
  Polygon* subject_poly = &info->subject->poly;
  float ox = subject_poly->ox;
  float oy = subject_poly->oy;
  find_bounds(subject_poly, false);
  float p = 1 - 1 / (subject_poly->max_y - subject_poly->min_y);
  subject_poly->ox = info->subject->poly.ox + info->vx;
  subject_poly->oy = info->subject->poly.oy + info->vy;
  PolygonIntersection m = polygon_intersection(entity_poly, subject_poly);

  if (m.intersects) {
    float ceiling = 1;
    float floor = 0;
    float fac = 0.5;
    float vx = info->vx;
    float vy = info->vy;
    while (ceiling - floor > 0.01) {
      subject_poly->ox -= vx;
      subject_poly->oy -= vy;
      vx = info->vx * fac;
      vy = info->vy * fac;
      subject_poly->ox += vx;
      subject_poly->oy += vy;
      m = polygon_intersection(entity_poly, subject_poly);
      info->subject->collision.pi = m;
      if (!m.intersects) {
        floor = fac;
      } else {
        ceiling = fac;
      }
      fac = floor + (ceiling - floor)/2;
    }
    info->vx = vx;
    info->vy = vy;
  }
  subject_poly->ox = ox;
  subject_poly->oy = oy;
  return true;
}

void move_entity(Entity* entity, float x, float y) {
  SubjectEntityInfo info = { .subject = entity, .vx = x, .vy = y };

  if (!entity->poly.count) {
    printf("we need a polygon for collision!!\n");
    return;
  }
  if (fabs(x) < 0.01) {
    x = 0;
    if (fabs(y) < 0.01 && !window.keys[KEY_SPACE]) {
      return;
    }
  } else if (fabs(y) < 0.01) {
    y = 0;
  }

  entity->collision.pi.intersects = false;
  visit_entities(collide_entities, &info);

  if (fabs(info.vx) >= 0.25) {
    entity->poly.ox += info.vx;
  }
  if (fabs(info.vy) >= 0.25) {
    entity->poly.oy += info.vy;
  }
}

void snap_entity_to_surface(Entity* subject, Entity* target, size_t subject_point, size_t target_edge) {
  float tx1 = target->poly.points[target_edge].x + target->poly.ox;
  float ty1 = target->poly.points[target_edge].y + target->poly.oy;
  float tx2 = target->poly.points[(target_edge + 1) % target->poly.count].x + target->poly.ox;
  float ty2 = target->poly.points[(target_edge + 1) % target->poly.count].y + target->poly.oy;
  float tx = tx2-tx1;
  float ty = ty2-ty1;
  float slope = ty/tx;
  float intercept = ty1 - slope * tx1;
  float subject_y = (subject->poly.points[subject_point].x + subject->poly.ox) * slope + intercept;
  subject->poly.oy = subject_y - subject->poly.points[subject_point].y;
}
