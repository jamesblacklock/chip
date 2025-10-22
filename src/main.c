#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#ifdef WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

#include "main.h"
#include "window.h"
#include "entity.h"
#include "graphics.h"
#include "polygon.h"

static bool init_map_editor();
static bool map_editor(size_t ms);
static bool nothing(size_t ms) {
  fprintf(stderr, "no mode selected\n");
  return false;
}

static char* app_path;
static uint32_t mode;
static bool (*tick_routine)(size_t) = nothing;

struct {
  const char* file;
  Entity* polygons[6000];
  ptrdiff_t polygon_index;
  ptrdiff_t redo_polygon_index;
} MapEditor;

void process_argv(int argc, char** argv) {
  if (argc < 2) {
    return;
  }
  if (strcmp(argv[1], "mapeditor") == 0) {
    if (2 >= argc) {
      printf("invalid parameters; expected file path following \"mapeditor\"\n");
      exit(1);
    }
    MapEditor.file = argv[2];
    tick_routine = map_editor;
  }
}

bool init(int argc, char** argv, const char* app_path0, VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height) {
  srand(time(NULL));

  size_t len = strlen(app_path0);
  app_path = malloc(len + 1);
  strncpy(app_path, app_path0, len);
  app_path[len] = 0;

  init_window(width, height);
  process_argv(argc, argv);
  return init_vulkan(instance, surface, width, height);
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

void cleanup() {
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

static bool map_editor_save() {
  FILE* fp = fopen(MapEditor.file, "wb");
  for (size_t i=0; i <= MapEditor.polygon_index; i++) {
    write_object(&MapEditor.polygons[i]->poly, fp);
  }
  write_eof(fp);
  fclose(fp);
  return true;
}

static bool map_editor_load() {
  FILE* fp = fopen(MapEditor.file, "rb");
  Polygon* poly = (Polygon*) read_object(fp);
  while (poly != NULL) {
    if (poly->__so.object_type != OBJECT_TYPE_POLYGON) {
      printf("encountered unexpected object: %d", poly->__so.object_type);
      continue;
    }
    if (validate_polygon(poly)) {
      Color color = { rand() / (float) RAND_MAX, rand() / (float) RAND_MAX, rand() / (float) RAND_MAX };
      MapEditor.polygons[++MapEditor.polygon_index] = create_entity((Entity){ .poly = *poly, .color = color });
      attach_body(MapEditor.polygons[MapEditor.polygon_index], false);
    }
    free(poly);
    poly = (Polygon*) read_object(fp);
  }
  MapEditor.redo_polygon_index = MapEditor.polygon_index;
  fclose(fp);
  return true;
}

static bool init_map_editor() {
  MapEditor.polygon_index = -1;
  MapEditor.redo_polygon_index = -1;
  init_entities();
  if (access(MapEditor.file, F_OK) == 0) {
    printf("loading existing map \"%s\"\n", MapEditor.file);
    map_editor_load();
  } else {
    printf("creating new map \"%s\"\n", MapEditor.file);
  }
  return true;
}

static bool mouse_pressed(uint32_t button) {
  return !window.last_frame_mouse_buttons[button] && window.mouse_buttons[button];
}

static bool key_pressed(uint32_t key) {
  return !window.last_frame_keys[key] && window.keys[key];
}

static bool drag_delta(float* x, float* y, uint32_t button) {
  static float _x[MOUSE_BUTTON_COUNT], _y[MOUSE_BUTTON_COUNT];
  if (mouse_pressed(button)) {
    _x[button] = window.mouse_x;
    _y[button] = window.mouse_y;
  }
  if (window.mouse_buttons[button]) {
    *x = window.mouse_x - _x[button];
    *y = window.mouse_y - _y[button];
    _x[button] = window.mouse_x;
    _y[button] = window.mouse_y;
    return true;
  } else {
    *x = *y = 0;
    return false;
  }
}

static bool map_editor(size_t ms) {
  static bool undo;
  static bool redo;
  static bool save;
  static bool closing;
  static bool init;

  if (!init && !init_map_editor()) {
    return false;
  }
  init = true;

  static float view_x, view_y, view_z;
  float drag_view_x, drag_view_y;
  drag_delta(&drag_view_x, &drag_view_y, MOUSE_MIDDLE);
  view_x += screen_to_z0(drag_view_x);
  view_y += screen_to_z0(drag_view_y);
  view_z = window.scroll_y < 0 ? (-window.scroll_y*window.scroll_y)/10 : sqrt(window.scroll_y * 2000);
  set_view_coords(view_x, view_y, view_z);

  const size_t MAX_POINTS = 100;
  static float points[MAX_POINTS][2];
  static float current_point[2];
  static ptrdiff_t point_index = -1;
  static ptrdiff_t redo_point_index = -1;

  float x = window_to_entity(screen_x_to_z0(window.mouse_x));
  float y = window_to_entity(screen_y_to_z0(window.mouse_y));

  float grid_snap = screen_to_z0(window.keys[KEY_LCTRL] ? 10 : window.keys[KEY_LALT] ? 0.01 : 2);
  float angle_snap = window.keys[KEY_LALT] ? M_PI/16 : M_PI/4;

  x = round(x / grid_snap) * grid_snap;
  y = round(y / grid_snap) * grid_snap;

  if (!undo && (window.keys[KEY_LCTRL] || window.keys[KEY_LMETA]) && !window.keys[KEY_LSHIFT] && window.keys[KEY_Z]) {
    if (point_index >= 0) {
      point_index--;
    } else if (MapEditor.polygon_index >= 0) {
      disable_entity(MapEditor.polygons[MapEditor.polygon_index--]);
    }
    window.keys[KEY_Z] = false; // TODO: fix the macOS issue where keyUp is not reported while Meta is held
  }
  if (!redo && (window.keys[KEY_LCTRL] || window.keys[KEY_LMETA]) && window.keys[KEY_LSHIFT] && window.keys[KEY_Z]) {
    if (MapEditor.polygon_index < MapEditor.redo_polygon_index) {
      enable_entity(MapEditor.polygons[++MapEditor.polygon_index]);
    } else if (point_index < redo_point_index) {
      point_index++;
    }
    window.keys[KEY_Z] = false; // TODO: fix the macOS issue where keyUp is not reported while Meta is held
  }
  if (!save && (window.keys[KEY_LCTRL] || window.keys[KEY_LMETA]) && window.keys[KEY_S]) {
    map_editor_save();
    window.keys[KEY_S] = false; // TODO: fix the macOS issue where keyUp is not reported while Meta is held
  }

  if (key_pressed(KEY_ESC)) {
    point_index = -1;
  }

  if (mouse_pressed(MOUSE_LEFT)) {
    if (closing) {
      point_index++;
      points[point_index][0] = points[0][0];
      points[point_index][1] = points[0][1];
      Polygon poly = create_polygon(points, point_index);
      if (validate_polygon(&poly)) {
        Color color = { rand() / (float) RAND_MAX, rand() / (float) RAND_MAX, rand() / (float) RAND_MAX };
        MapEditor.polygons[++MapEditor.polygon_index] = create_entity((Entity){ .poly = poly, .color = color });
        attach_body(MapEditor.polygons[MapEditor.polygon_index], false);
      } else {
        free_polygon(&poly);
      }
      point_index = -1;
      closing = false;
    } else {
      if (point_index < 0) {
        points[0][0] = current_point[0] = x;
        points[0][1] = current_point[1] = y;
      }
      float prev_x = points[point_index][0];
      float prev_y = points[point_index][1];
      float dx = x - prev_x;
      float dy = y - prev_y;
      float dist_xy = sqrt(dx*dx + dy*dy);
      if (dist_xy > 1) {
        point_index++;
        points[point_index][0] = current_point[0];
        points[point_index][1] = current_point[1];
      }
    }
    redo_point_index = point_index;
    MapEditor.redo_polygon_index = MapEditor.polygon_index;
  }
  if (point_index >= 0) {
    if (window.keys[KEY_LSHIFT]) {
      float prev_x = points[point_index][0];
      float prev_y = points[point_index][1];
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

    float close_dist = screen_to_z0(5);
    closing = point_index > 1 && !window.keys[KEY_LALT] && fabsf(x - points[0][0]) < close_dist && fabsf(y - points[0][1]) < close_dist;
    if (closing) {
      x = points[0][0];
      y = points[0][1];
    }
    current_point[0] = x;
    current_point[1] = y;
  }

  undo = (window.keys[KEY_LCTRL] || window.keys[KEY_LMETA]) && !window.keys[KEY_LSHIFT] && window.keys[KEY_Z];
  redo = (window.keys[KEY_LCTRL] || window.keys[KEY_LMETA]) && window.keys[KEY_LSHIFT] && window.keys[KEY_Z];
  save = (window.keys[KEY_LCTRL] || window.keys[KEY_LMETA]) && window.keys[KEY_S];

  // render
  begin_render();
  render_entities();
  if (point_index >= 0) {
    for (ptrdiff_t i=0; i < point_index; i++) {
      draw_line((LineData){
        .x1 = entity_to_screen(points[i][0]),
        .y1 = entity_to_screen(points[i][1]),
        .x2 = entity_to_screen(points[i+1][0]),
        .y2 = entity_to_screen(points[i+1][1]),
        .w = screen_to_z0(entity_to_screen(1)),
        .r = 1,
      });
    }
    draw_line((LineData){
      .x1 = entity_to_screen(points[point_index][0]),
      .y1 = entity_to_screen(points[point_index][1]),
      .x2 = entity_to_screen(current_point[0]),
      .y2 = entity_to_screen(current_point[1]),
      .w = screen_to_z0(entity_to_screen(1)),
      .r = 1,
    });
  }
  end_render();

  return !window.closed;
}

bool tick(size_t ms) {
  if (window.keys[KEY_LMETA] && (window.keys[KEY_Q] || window.keys[KEY_W])) {
    window.closed = true;
  }
  bool res = tick_routine(ms);
  memcpy(window.last_frame_mouse_buttons, window.mouse_buttons, sizeof(window.mouse_buttons));
  memcpy(window.last_frame_keys, window.keys, sizeof(window.keys));
  return res;
}
