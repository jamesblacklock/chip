#ifdef WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif
#include <string.h>

#include "main.h"
#include "graphics.h"
#include "window.h"
#include "entity.h"

static const char* g_filename;
static Entity* g_polygons[6000];
static ptrdiff_t g_polygon_index;
static ptrdiff_t g_redo_polygon_index;

static bool init(ExeArgs args) {
  g_filename = args.map_editor.filename;
  g_polygon_index = -1;
  g_redo_polygon_index = -1;
  init_entities();
  if (access(g_filename, F_OK) == 0) {
    printf("loading existing map \"%s\"\n", g_filename);
    Map map = load_map_file(g_filename);
    memcpy(g_polygons, map.polygons, sizeof(Entity*) * map.polygon_count);
    map.polygons = NULL;
    free_map(&map);
    g_redo_polygon_index = g_polygon_index = map.polygon_count - 1;
  } else {
    printf("creating new map \"%s\"\n", g_filename);
  }
  return true;
}

static bool map_editor_save() {
  Map map = { .polygon_count = g_polygon_index + 1, .polygons = g_polygons };
  return save_map_file(g_filename, &map);
}

static bool tick(float ms) {
  static bool undo;
  static bool redo;
  static bool save;
  static bool closing;

  static float view_x, view_y, view_z = NEUTRAL_Z_DIST;
  float drag_view_x, drag_view_y;
  drag_delta(&drag_view_x, &drag_view_y, MOUSE_MIDDLE);
  view_x += screen_to_z0(drag_view_x);
  view_y += screen_to_z0(drag_view_y);
  view_z += view_z * window.scroll_y_delta / 100;
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
    } else if (g_polygon_index >= 0) {
      disable_entity(g_polygons[g_polygon_index--]);
    }
    window.keys[KEY_Z] = false; // TODO: fix the macOS issue where keyUp is not reported while Meta is held
  }
  if (!redo && (window.keys[KEY_LCTRL] || window.keys[KEY_LMETA]) && window.keys[KEY_LSHIFT] && window.keys[KEY_Z]) {
    if (g_polygon_index < g_redo_polygon_index) {
      enable_entity(g_polygons[++g_polygon_index]);
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
        g_polygons[++g_polygon_index] = create_entity((Entity){ .poly = poly, .color = color });
        attach_body(g_polygons[g_polygon_index], false);
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
    g_redo_polygon_index = g_polygon_index;
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

Program program_map_editor = {
  .init = init,
  .tick = tick,
};
