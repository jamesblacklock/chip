#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>

#include "main.h"
#include "window.h"
#include "entity.h"
#include "graphics.h"

static char* app_path;

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

// void update_entity(Entity* entity, void* _data) {
//   EntityUpdateData* data = _data;
//   if (entity == data->bloop) {
//     return;
//   }
//   if (fabsf(entity->velocity.x) < 0.001) {
//     entity->velocity.x = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//     entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (fabsf(entity->velocity.y) < 0.001) {
//     entity->velocity.y = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//     entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.x > 0 && entity_to_window(entity->x) > window.width/2) {
//     entity->velocity.x = -entity->velocity.x;
//     entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.x < 0 && entity_to_window(-entity->x) > window.width/2) {
//     entity->velocity.x = -entity->velocity.x;
//     entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.y > 0 && entity_to_window(entity->y) > window.height/2) {
//     entity->velocity.y = -entity->velocity.y;
//     entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.y < 0 && entity_to_window(-entity->y) > window.height/2) {
//     entity->velocity.y = -entity->velocity.y;
//     entity->velocity.a = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//   }
//   entity->x += entity->velocity.x * data->ms;
//   entity->y += entity->velocity.y * data->ms;
//   entity->angle += entity->velocity.a * data->ms;
// }

static float dist(float x, float y) {
  return sqrt(x*x + y*y);
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
    bloop = create_entity((Entity){ .w = 1, .h = 1, .x = x, .y = y, .color = {r,g,b} });
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
    // // DO NOT DELETE -- drawing a line
    // float x1 = window_to_entity(drag_x);
    // float x2 = window_to_entity(window.mouse_x);
    // float y1 = window_to_entity(drag_y);
    // float y2 = window_to_entity(window.mouse_y);
    // float dx = x2-x1;
    // float dy = y2-y1;
    // bloop->angle = atan(dy/dx);
    // if (window.keys[KEY_LSHIFT]) {
    //   bloop->angle = round(bloop->angle / (M_PI/8)) * (M_PI/8);
    //   float dist_xy = dist(dx, dy);
    //   float x_sign = dx < 0 ? -1 : 1;
    //   float y_sign = dy < 0 ? -1 : 1;
    //   dx = fabs(cos(bloop->angle)) * dist_xy * x_sign;
    //   dy = fabs(sin(bloop->angle)) * dist_xy * y_sign;
    // }
    // bloop->w = dist(dx, dy);
    // bloop->x = dx/2 + x1;
    // bloop->y = dy/2 + y1;
  }
  if (!window.mouse_left && bloop) {
    attach_body(bloop, window.keys[KEY_LSHIFT]);
    bloop = NULL;
  }
  m = window.mouse_left;
  
  // visit_entities(update_entity, &(EntityUpdateData){ .bloop = bloop, .ms = ms });

  b2World_Step(world, 1.0/60.0, 10);
  update_entities();

  begin_render();
  draw_triangle((TriangleData){
    .x1 = 0, .y1 = 0, .x2 = 0.8, .y2 = 0.5, .x3 = 1, .y3 = 0,
    .r = 1, .g = 1,
  });
  render_entities();
  end_render();

  if (window.keys[KEY_LMETA] && (window.keys[KEY_Q] || window.keys[KEY_W])) {
    window.closed = true;
  }
  return !window.closed;
}
