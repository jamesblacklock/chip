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

static void box_drop() {
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
  }
  if (!window.mouse_left && bloop) {
    attach_body(bloop, window.keys[KEY_LSHIFT]);
    bloop = NULL;
  }
  m = window.mouse_left;

  b2World_Step(world, 1.0/60.0, 10);
  update_entities();
}

typedef struct EntityUpdateData {
  Entity* bloop;
  size_t ms;
} EntityUpdateData;

static void box_screensaver_update_entity(Entity* entity, void* _data) {
  EntityUpdateData* data = _data;
  if (entity == data->bloop) {
    return;
  }
  if (fabsf(entity->velocity.x) < 0.001) {
    entity->velocity.x = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
    entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
  }
  if (fabsf(entity->velocity.y) < 0.001) {
    entity->velocity.y = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
    entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.x > 0 && entity_to_screen(entity->x) > window.width/2) {
    entity->velocity.x = -entity->velocity.x;
    entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.x < 0 && entity_to_screen(-entity->x) > window.width/2) {
    entity->velocity.x = -entity->velocity.x;
    entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.y > 0 && entity_to_screen(entity->y) > window.height/2) {
    entity->velocity.y = -entity->velocity.y;
    entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
  }
  if (entity->velocity.y < 0 && entity_to_screen(-entity->y) > window.height/2) {
    entity->velocity.y = -entity->velocity.y;
    entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
  }
  entity->x += entity->velocity.x * data->ms;
  entity->y += entity->velocity.y * data->ms;
  entity->angle += entity->velocity.a * data->ms;
}

static void box_screensaver(size_t ms) {
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
  }
  if (!window.mouse_left && bloop) {
    bloop = NULL;
  }
  m = window.mouse_left;
  
  visit_entities(box_screensaver_update_entity, &(EntityUpdateData){ .bloop = bloop, .ms = ms });
}

static void draw_polygons() {
  typedef struct Polygon {
    float* points;
    size_t count;
  } Polygon;

  static Polygon polys[100];
  static size_t poly_index;
  static bool mouse_down = false;
  static bool closing = false;
  const size_t MAX_POINTS = 100;
  static float points[MAX_POINTS][2];
  static ptrdiff_t point_index = -1;

  float x = window_to_entity(window.mouse_x);
  float y = window_to_entity(window.mouse_y);

  float grid_snap = window.keys[KEY_LALT] ? 5 : 10;
  float angle_snap = window.keys[KEY_LALT] ? M_PI/16 : M_PI/4;

  if (window.keys[KEY_LCTRL]) {
    x = round(x / grid_snap) * grid_snap;
    y = round(y / grid_snap) * grid_snap;
  }

  if (!mouse_down && window.mouse_left) {
    if (closing) {
      size_t count = point_index;
      Polygon p = {
        .count = count,
        .points = malloc(sizeof(float) * count * 2),
      };
      for (size_t i=0; i < count; i++) {
        p.points[i*2] = points[i][0];
        p.points[i*2+1] = points[i][1];
      }
      polys[poly_index++] = p;
      point_index = -1;
      closing = false;
    } else {
      if (point_index < 0) {
        points[0][0] = points[1][0] = x;
        points[0][1] = points[1][1] = y;
        point_index = 0;
      }
      point_index++;
    }
  }
  if (point_index > 0) {
    if (window.keys[KEY_LSHIFT]) {
      float prev_x = points[point_index-1][0];
      float prev_y = points[point_index-1][1];
      float dx = x - prev_x;
      float dy = y - prev_y;
      float angle = atan(dy/dx);
      angle = round(angle / angle_snap) * angle_snap;
      float dist_xy = sqrt(dx*dx + dy*dy);
      float x_sign = dx < 0 ? -1 : 1;
      float y_sign = dy < 0 ? -1 : 1;
      dx = fabs(cos(angle)) * dist_xy * x_sign;
      dy = fabs(sin(angle)) * dist_xy * y_sign;
      x = dx + prev_x;
      y = dy + prev_y;
    }

    closing = point_index > 2 && !window.keys[KEY_LALT] && fabsf(x - points[0][0]) < 6 && fabsf(y - points[0][1]) < 6;
    if (closing) {
      x = points[0][0];
      y = points[0][1];
    }
    points[point_index][0] = x;
    points[point_index][1] = y;
  }
  mouse_down = window.mouse_left;

  if (point_index > 0) {
    for (size_t i=0; i < point_index; i++) {
      float x1 = entity_to_screen(points[i][0]);
      float x2 = entity_to_screen(points[i+1][0]);
      float y1 = entity_to_screen(points[i][1]);
      float y2 = entity_to_screen(points[i+1][1]);
      float p1[2], p2[2];
      screen_to_world(x1, y1, p1);
      screen_to_world(x2, y2, p2);
      draw_line((LineData){
        .x1 = p1[0],
        .y1 = p1[1],
        .x2 = p2[0],
        .y2 = p2[1],
        .w = screen_x_to_world(entity_to_screen(1)),
        .r = 1, .g = 1,
      });
    }
  }
  for (size_t i=0; i < poly_index; i++) {
    for (size_t j=0; j < polys[i].count; j++) {
      size_t next_index = (j+1) % polys[i].count;
      float x1 = entity_to_screen(polys[i].points[j*2]);
      float x2 = entity_to_screen(polys[i].points[next_index*2]);
      float y1 = entity_to_screen(polys[i].points[j*2+1]);
      float y2 = entity_to_screen(polys[i].points[next_index*2+1]);
      float p1[2], p2[2];
      screen_to_world(x1, y1, p1);
      screen_to_world(x2, y2, p2);
      draw_line((LineData){
        .x1 = p1[0],
        .y1 = p1[1],
        .x2 = p2[0],
        .y2 = p2[1],
        .w = screen_x_to_world(entity_to_screen(1)),
        .r = 1, .g = 1,
      });
    }
  }
}

bool tick(size_t ms) {
  if (window.closed) {
    return false;
  }

  begin_render();
  draw_polygons();
  render_entities();
  end_render();

  if (window.keys[KEY_LMETA] && (window.keys[KEY_Q] || window.keys[KEY_W])) {
    window.closed = true;
  }
  return !window.closed;
}
