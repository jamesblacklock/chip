#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
// #include <chipmunk/chipmunk.h>

#include "main.h"
#include "window.h"
#include "entity.h"
#include "graphics.h"

char* app_path;

bool init(const char* app_path0, VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height) {
  size_t len = strlen(app_path0);
  app_path = malloc(len + 1);
  strncpy(app_path, app_path0, len);
  app_path[len] = 0;

  init_window(width, height);
  init_entities();
  return init_vulkan(instance, surface, width, height);
}

byte* read_file(const char* filename, size_t* size) {
  char* absolute_path = malloc(strlen(filename) + strlen(app_path) + 1);
  absolute_path[0] = 0;
  strcat(absolute_path, app_path);
  strcat(absolute_path, filename);

  struct stat info;
  int res = stat(absolute_path, &info);
  if (res != 0) {
    fprintf(stderr, "failed to stat file: %s\n", absolute_path);
    exit(1);
  }
  byte* buf = malloc(info.st_size);
  FILE* file = fopen(absolute_path, "rb");
  if (file == NULL) {
    fprintf(stderr, "failed to open file: %s\n", absolute_path);
    exit(1);
  }
  size_t bytes_read = fread(buf, 1, info.st_size, file);
  if (bytes_read != info.st_size) {
    fprintf(stderr, "failed to read file: %s\n", absolute_path);
    exit(1);
  }
  *size = bytes_read;
  return buf;
}

void cleanup() {
  cleanup_vulkan();
}

typedef struct EntityUpdateData {
  Entity* bloop;
  size_t ms;
} EntityUpdateData;

void update_entity(Entity* entity, void* _data) {
  EntityUpdateData* data = _data;
  if (entity == data->bloop) {
    return;
  }
  if (fabsf(entity->velocity.x) < 0.001) {
    entity->velocity.x = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
    entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
  }
  if (fabsf(entity->velocity.y) < 0.001) {
    entity->velocity.y = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
    entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.x > 0 && entity_to_window(entity->x) > window.width/2) {
    entity->velocity.x = -entity->velocity.x;
    entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.x < 0 && entity_to_window(-entity->x) > window.width/2) {
    entity->velocity.x = -entity->velocity.x;
    entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.y > 0 && entity_to_window(entity->y) > window.height/2) {
    entity->velocity.y = -entity->velocity.y;
    entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.y < 0 && entity_to_window(-entity->y) > window.height/2) {
    entity->velocity.y = -entity->velocity.y;
    entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
  }
  entity->x += entity->velocity.x * data->ms;
  entity->y += entity->velocity.y * data->ms;
  entity->angle += entity->velocity.a * data->ms;
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

bool tick(size_t ms) {
  if (window.closed) {
    return false;
  }

  static bool m = false;
  static float drag_x;
  static float drag_y;
  static Entity* bloop = NULL;
  if (!m && window.mouse_left) {
    drag_x = window.mouse_x;
    drag_y = window.mouse_y;
    m = true;
    float r = rand() / (float) RAND_MAX;
    float g = rand() / (float) RAND_MAX;
    float b = rand() / (float) RAND_MAX;
    float x = window_to_entity(drag_x);
    float y = window_to_entity(drag_y);
    bloop = create_entity((Entity){ .w = 0, .h = 0, .x = x, .y = y, .color = {r,g,b} });
  }
  if (m && window.mouse_left && bloop) {
    float x1 = window_to_entity(fmin(window.mouse_x, drag_x));
    float x2 = window_to_entity(fmax(window.mouse_x, drag_x));
    float y1 = window_to_entity(fmin(window.mouse_y, drag_y));
    float y2 = window_to_entity(fmax(window.mouse_y, drag_y));
    bloop->w = x2 - x1;
    bloop->x = x1 + bloop->w/2;
    bloop->h = y2 - y1;
    bloop->y = y1 + bloop->h/2;
  }
  if (!window.mouse_left) {
    bloop = NULL;
  }
  m = window.mouse_left;
  
  visit_entities(update_entity, &(EntityUpdateData){ .bloop = bloop, .ms = ms });


  begin_render();
  visit_entities(render_entity, NULL);
  end_render();

  if (window.keys[KEY_LMETA] && (window.keys[KEY_Q] || window.keys[KEY_W])) {
    window.closed = true;
  }
  return !window.closed;
}
