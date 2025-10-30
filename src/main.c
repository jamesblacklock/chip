#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include "main.h"
#include "window.h"
#include "graphics.h"
#include "polygon.h"
#include "map_editor.h"
#include "play.h"

static char* app_path;
static Program program;

static Program program_test;

ExeArgs process_argv(int argc, char** argv) {
  ExeArgs args = { .valid = false };
  if (argc < 2) {
    return args;
  }
  if (strcmp(argv[1], "edit") == 0) {
    if (2 >= argc) {
      printf("invalid parameters; expected file path following \"edit\"\n");
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
  } else if (strcmp(argv[1], "test") == 0) {
    program = program_test;
    args.valid = true;
  } else {
    printf("invalid parameters; valid program selections include:\n- edit\n- play\n");
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

void mouse_viewport() {
  static float view_x, view_y, view_z = NEUTRAL_Z_DIST;
  float drag_view_x, drag_view_y;
  drag_delta(&drag_view_x, &drag_view_y, MOUSE_MIDDLE);
  view_x -= screen_to_z0(drag_view_x);
  view_y -= screen_to_z0(drag_view_y);
  view_z += view_z * window.scroll_y_delta / 100;
  set_view_coords(view_x, view_y, view_z);
}

SerializableObject* read_object(FILE* fp) {
  int16_t type;
  fread(&type, sizeof(int16_t), 1, fp);
  switch (type) {
    case OBJECT_TYPE_POLYGON: {
      float x, y, z;
      fread(&x, sizeof(float), 1, fp);
      fread(&y, sizeof(float), 1, fp);
      fread(&z, sizeof(float), 1, fp);
      uint16_t point_count;
      fread(&point_count, sizeof(uint16_t), 1, fp);
      if (point_count > 1000) {
        fprintf(stderr, "encountered invalid polygon point count: %u\n", point_count);
        return NULL;
      }
      Polygon* polygon = malloc(sizeof(Polygon));
      Vec2* points = malloc(sizeof(Vec2) * point_count);
      fread(points, sizeof(Vec2), point_count, fp);
      *polygon = polygon_new(points, point_count);
      free(points);
      polygon->x = x;
      polygon->y = y;
      polygon->z = z;
      polygon_position_changed(polygon);
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
      fwrite(&poly->x, sizeof(float), 1, fp);
      fwrite(&poly->y, sizeof(float), 1, fp);
      fwrite(&poly->z, sizeof(float), 1, fp);
      uint16_t point_count = poly->count;
      fwrite(&point_count, sizeof(uint16_t), 1, fp);
      fwrite(poly->points, sizeof(Vec2), point_count, fp);
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
  memcpy(window.last_frame_keys, window.keys_repeating, sizeof(window.keys_repeating));
  window.scroll_x_delta = 0;
  window.scroll_y_delta = 0;
  return res;
}

bool test_init(ExeArgs args) {
  return true;
}

bool test_tick(float ms) {
  return !window.closed;
}

static Program program_test = {
  .init = test_init,
  .tick = test_tick,
};
