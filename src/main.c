#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include "main.h"
#include "window.h"
#include "entity.h"
#include "graphics.h"
#include "polygon.h"
#include "map_editor.h"
#include "play.h"

static char* app_path;
static Program program;

ExeArgs process_argv(int argc, char** argv) {
  ExeArgs args = { .valid = false };
  if (argc < 2) {
    return args;
  }
  if (strcmp(argv[1], "mapeditor") == 0) {
    if (2 >= argc) {
      printf("invalid parameters; expected file path following \"mapeditor\"\n");
    }
    program = program_map_editor;
    args.map_editor.filename = argv[2];
    args.valid = true;
  } else if (strcmp(argv[1], "play") == 0) {
    if (2 >= argc) {
      printf("invalid parameters; expected file path following \"play\"\n");
    }
    program = program_play;
    args.play.mapfile = argv[2];
    args.valid = true;
  } else {
    printf("invalid parameters; valid program selections include:\n- mapeditor\n- play\n");
  }
  return args;
}

bool host_init(int argc, char** argv, const char* app_path0, VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height) {
  srand(time(NULL));

  size_t len = strlen(app_path0);
  app_path = malloc(len + 1);
  strncpy(app_path, app_path0, len);
  app_path[len] = 0;

  init_window(width, height);
  ExeArgs args = process_argv(argc, argv);
  if (!args.valid) {
    return false;
  }
  if (!init_vulkan(instance, surface, width, height)) {
    return false;
  }
  if (program.init) {
    if (!program.init(args)) {
      return false;
    }
  }
  return true;
}

byte* read_file(const char* filename, size_t* size) {
  struct stat info;
  int res = stat(filename, &info);
  if (res != 0) {
    fprintf(stderr, "failed to stat file: %s\n", filename);
    exit(1);
  }
  byte* buf = malloc(info.st_size);
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    fprintf(stderr, "failed to open file: %s\n", filename);
    exit(1);
  }
  size_t bytes_read = fread(buf, 1, info.st_size, file);
  if (bytes_read != info.st_size) {
    fprintf(stderr, "failed to read file: %s\n", filename);
    exit(1);
  }
  if (size != NULL) {
    *size = bytes_read;
  }
  fclose(file);
  return buf;
}

byte* load_app_resource(const char* filename, size_t* size) {
  char* absolute_path = malloc(strlen(filename) + strlen(app_path) + 1);
  absolute_path[0] = 0;
  strcat(absolute_path, app_path);
  strcat(absolute_path, filename);
  return read_file(absolute_path, size);
}

void host_cleanup() {
  cleanup_vulkan();
}

static void box_drop() {
  static bool m = false;
  static float drag_x;
  static float drag_y;
  static Entity* bloop = NULL;
  if (!m && window.mouse_buttons[MOUSE_LEFT]) {
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
  if (m && window.mouse_buttons[MOUSE_LEFT] && bloop) {
    float x1 = window_to_entity(fmin(window.mouse_x, drag_x));
    float x2 = window_to_entity(fmax(window.mouse_x, drag_x));
    float y1 = window_to_entity(fmin(window.mouse_y, drag_y));
    float y2 = window_to_entity(fmax(window.mouse_y, drag_y));
    bloop->w = x2 - x1;
    bloop->x = x1 + bloop->w/2;
    bloop->h = y2 - y1;
    bloop->y = y1 + bloop->h/2;
  }
  if (!window.mouse_buttons[MOUSE_LEFT] && bloop) {
    attach_body(bloop, window.keys[KEY_LSHIFT]);
    bloop = NULL;
  }
  m = window.mouse_buttons[MOUSE_LEFT];

  b2World_Step(world, 1.0/60.0, 10);
  update_entities();
}

static void box_spawn() {
  static bool m = false;
  static float drag_x;
  static float drag_y;
  if (!m && window.mouse_buttons[MOUSE_RIGHT]) {
    drag_x = window.mouse_x;
    drag_y = window.mouse_y;
    m = true;
    float r = rand() / (float) RAND_MAX;
    float g = rand() / (float) RAND_MAX;
    float b = rand() / (float) RAND_MAX;
    float x = window_to_entity(window.mouse_x);
    float y = window_to_entity(window.mouse_y);
    Entity* entity = create_entity((Entity){ .w = 10, .h = 10, .x = x, .y = y, .color = {r,g,b} });
    attach_body(entity, true);
  }
  m = window.mouse_buttons[MOUSE_RIGHT];
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
  if (!m && window.mouse_buttons[MOUSE_LEFT]) {
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
  if (m && window.mouse_buttons[MOUSE_LEFT] && bloop) {
    float x1 = window_to_entity(fmin(window.mouse_x, drag_x));
    float x2 = window_to_entity(fmax(window.mouse_x, drag_x));
    float y1 = window_to_entity(fmin(window.mouse_y, drag_y));
    float y2 = window_to_entity(fmax(window.mouse_y, drag_y));
    bloop->w = x2 - x1;
    bloop->x = x1 + bloop->w/2;
    bloop->h = y2 - y1;
    bloop->y = y1 + bloop->h/2;
  }
  if (!window.mouse_buttons[MOUSE_LEFT] && bloop) {
    bloop = NULL;
  }
  m = window.mouse_buttons[MOUSE_LEFT];
  
  visit_entities(box_screensaver_update_entity, &(EntityUpdateData){ .bloop = bloop, .ms = ms });
}

SerializableObject* read_object(FILE* fp) {
  int16_t type;
  fread(&type, sizeof(int16_t), 1, fp);
  switch (type) {
    case OBJECT_TYPE_POLYGON: {
      uint16_t point_count;
      fread(&point_count, sizeof(uint16_t), 1, fp);
      if (point_count > 1000) {
        fprintf(stderr, "encountered invalid polygon point count: %u\n", point_count);
        return NULL;
      }
      Polygon* polygon = malloc(sizeof(Polygon));
      Point* points = malloc(sizeof(Point) * point_count);
      fread(points, sizeof(Point), point_count, fp);
      *polygon = create_polygon((void*)points, point_count);
      free(points);
      printf("read polygon with %u points\n", point_count);
      return (SerializableObject*) polygon;
    }
    case OBJECT_TYPE_EOF: {
      printf("read end-of-file marker\n\n");
      return NULL;
    }
    default: {
      fprintf(stderr, "encountered invalid object type: %d\n", type);
      return NULL;
    }
  }
}

void write_eof(FILE* fp) {
  int16_t type = OBJECT_TYPE_EOF;
  fwrite(&type, sizeof(int16_t), 1, fp);
  printf("wrote end-of-file marker\n\n");
}

void write_object(void* object, FILE* fp) {
  SerializableObject* object_ = object;
  fwrite(&object_->object_type, sizeof(int16_t), 1, fp);
  switch (object_->object_type) {
    case OBJECT_TYPE_POLYGON: {
      Polygon* poly = (Polygon*) object;
      uint16_t point_count = poly->count;
      fwrite(&point_count, sizeof(uint16_t), 1, fp);
      fwrite(poly->points, sizeof(Point), point_count, fp);
      printf("wrote polygon with %u points\n", point_count);
      return;
    }
    case OBJECT_TYPE_EOF: {
      printf("wrote end-of-file marker\n");
      return;
    }
    default: {
      fprintf(stderr, "encountered invalid object type: %d\n", object_->object_type);
      return;
    }
  }
}

bool host_tick(float ms) {
  if (window.keys[KEY_LMETA] && (window.keys[KEY_Q] || window.keys[KEY_W])) {
    window.closed = true;
  }
  bool res = program.tick(ms);
  memcpy(window.last_frame_mouse_buttons, window.mouse_buttons, sizeof(window.mouse_buttons));
  memcpy(window.last_frame_keys, window.keys, sizeof(window.keys));
  window.scroll_x_delta = 0;
  window.scroll_y_delta = 0;
  return res;
}
