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

static Program program_test;

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
  } else if (strcmp(argv[1], "test") == 0) {
    program = program_test;
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

void mouse_viewport() {
  static float view_x, view_y, view_z = NEUTRAL_Z_DIST;
  float drag_view_x, drag_view_y;
  drag_delta(&drag_view_x, &drag_view_y, MOUSE_MIDDLE);
  view_x -= screen_to_z0(drag_view_x);
  view_y -= screen_to_z0(drag_view_y);
  view_z += view_z * window.scroll_y_delta / 100;
  set_view_coords(view_x, view_y, view_z);
}

// typedef struct EntityUpdateData {
//   Entity* bloop;
//   size_t ms;
// } EntityUpdateData;

// static void box_screensaver_update_entity(Entity* entity, void* _data) {
//   EntityUpdateData* data = _data;
//   if (entity == data->bloop) {
//     return;
//   }
//   if (fabsf(entity->velocity.x) < 0.001) {
//     entity->velocity.x = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//     entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (fabsf(entity->velocity.y) < 0.001) {
//     entity->velocity.y = (rand() % 10) / 50.0 * (rand() % 2 ? 1 : -1);
//     entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.x > 0 && entity_to_screen(entity->x) > window.width/2) {
//     entity->velocity.x = -entity->velocity.x;
//     entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.x < 0 && entity_to_screen(-entity->x) > window.width/2) {
//     entity->velocity.x = -entity->velocity.x;
//     entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.y > 0 && entity_to_screen(entity->y) > window.height/2) {
//     entity->velocity.y = -entity->velocity.y;
//     entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
//   }
//   if (entity->velocity.y < 0 && entity_to_screen(-entity->y) > window.height/2) {
//     entity->velocity.y = -entity->velocity.y;
//     entity->velocity.a = (rand() % 10) / 3000.0 * (rand() % 2 ? 1 : -1);
//   }
//   entity->x += entity->velocity.x * data->ms;
//   entity->y += entity->velocity.y * data->ms;
//   entity->angle += entity->velocity.a * data->ms;
// }

// static void box_screensaver(size_t ms) {
//   static bool m = false;
//   static float drag_x;
//   static float drag_y;
//   static Entity* bloop = NULL;
//   if (!m && window.mouse_buttons[MOUSE_LEFT]) {
//     drag_x = window.mouse_x;
//     drag_y = window.mouse_y;
//     m = true;
//     float r = rand() / (float) RAND_MAX;
//     float g = rand() / (float) RAND_MAX;
//     float b = rand() / (float) RAND_MAX;
//     float x = window_to_entity(drag_x);
//     float y = window_to_entity(drag_y);
//     bloop = create_entity((Entity){ .w = 1, .h = 1, .x = x, .y = y, .color = {r,g,b} });
//   }
//   if (m && window.mouse_buttons[MOUSE_LEFT] && bloop) {
//     float x1 = window_to_entity(fmin(window.mouse_x, drag_x));
//     float x2 = window_to_entity(fmax(window.mouse_x, drag_x));
//     float y1 = window_to_entity(fmin(window.mouse_y, drag_y));
//     float y2 = window_to_entity(fmax(window.mouse_y, drag_y));
//     bloop->w = x2 - x1;
//     bloop->x = x1 + bloop->w/2;
//     bloop->h = y2 - y1;
//     bloop->y = y1 + bloop->h/2;
//   }
//   if (!window.mouse_buttons[MOUSE_LEFT] && bloop) {
//     bloop = NULL;
//   }
//   m = window.mouse_buttons[MOUSE_LEFT];
  
//   visit_entities(box_screensaver_update_entity, &(EntityUpdateData){ .bloop = bloop, .ms = ms });
// }

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
      Vec2* points = malloc(sizeof(Vec2) * point_count);
      fread(points, sizeof(Vec2), point_count, fp);
      *polygon = create_polygon(points, point_count);
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

typedef struct Foot {
  float x1;
  float x2;
} Foot;

typedef struct Floor {
  Entity* entity;
  uint32_t edge;
  float x;
  bool state;
} Floor;

typedef struct Robot {
  float x;
  // float y;
  float vx;
  float vy;
  // float w;
  float h;
  // float angle;
  Foot foot;
  float floor_y;
  float floor_angle;
  float target_floor_angle;
  Floor floor;
  bool collided;
} Robot;

struct {
  Robot robot;
} g;

bool test_init(ExeArgs args) {
  g.robot = (Robot){
    // .w = 36,
    .h = 60,
    .x = -100,
    .floor_y = -100,
    .foot = {.x1 = -14, .x2 = 18},
  };
  load_map_file("newmap.map");
  return true;
}

void render_robot(Robot* robot) {
  draw_line((LineData){
    .w = entity_to_screen(1),
    .x1 = entity_to_screen(robot->x + cos(robot->floor_angle) * robot->foot.x1),
    .y1 = entity_to_screen(robot->floor_y + sin(robot->floor_angle) * robot->foot.x1),
    .x2 = entity_to_screen(robot->x + cos(robot->floor_angle) * robot->foot.x2),
    .y2 = entity_to_screen(robot->floor_y + sin(robot->floor_angle) * robot->foot.x2),
    .r = 1, .g = 1,
  });
  draw_line((LineData){
    .w = entity_to_screen(1),
    .x1 = entity_to_screen(robot->x),
    .y1 = entity_to_screen(robot->floor_y),
    .x2 = entity_to_screen(robot->x),
    .y2 = entity_to_screen(robot->floor_y - robot->h),
    .r = 1, .g = 1,
  });
}

bool snap_foot_to_surface(Robot* robot, Entity* target, size_t target_edge) {
  Edge* edge = &target->poly.edges[target_edge];

  if (!between(edge->normal, M_PI*5/4, M_PI*7/4)) {
    return false;
  }

  float robot_x = robot->x;
  if (robot->floor.state && robot->floor.entity == target && robot->floor.edge == target_edge) {
    robot_x = edge->x1 + robot->floor.x + robot->vx;
  }

  float subject_y = robot_x * edge->slope + edge->intercept;

  float foot_x1 = robot_x + cos(robot->floor_angle) * robot->foot.x1;
  float foot_x2 = robot_x + cos(robot->floor_angle) * robot->foot.x2;

  if (robot_x + robot->foot.x2 >= edge->x1 && robot_x + robot->foot.x1 < edge->x2) {
    robot->floor_y = subject_y;
    robot->target_floor_angle = edge->angle;
    robot->floor.state = true;
    robot->floor.entity = target;
    robot->floor.edge = target_edge;
    robot->floor.x = robot_x - edge->x1;
    robot->x = robot_x;
    return true;
  }
  if (robot->floor.state && robot->floor.entity == target && robot->floor.edge == target_edge) {
    robot->floor.state = false;
  }
  return false;
}

static inline float pdist(float angle, float target) {
  return angle == target ? 0 : angle < target ? target - angle : M_PI*2 - angle + target;
}
static inline float ndist(float angle, float target) {
  return angle == target ? 0 : angle > target ? angle - target : angle + M_PI*2 - target;
}

void update_robot(Robot* robot, float ms) {
  if (g.robot.floor_angle != g.robot.target_floor_angle) {
    float positive_dist = pdist(g.robot.floor_angle, g.robot.target_floor_angle);
    float negative_dist = ndist(g.robot.floor_angle, g.robot.target_floor_angle);
    float sign = positive_dist - negative_dist < 0 ? 1 : -1;
    g.robot.floor_angle = g.robot.floor_angle + 0.006 * ms * sign;
    if (g.robot.floor_angle >= M_PI*2 || g.robot.floor_angle < 0) {
      g.robot.floor_angle -= M_PI*2 * sign;
    }
    positive_dist = pdist(g.robot.floor_angle, g.robot.target_floor_angle);
    negative_dist = ndist(g.robot.floor_angle, g.robot.target_floor_angle);
    if (sign != (positive_dist - negative_dist < 0 ? 1 : -1)) {
      g.robot.floor_angle = g.robot.target_floor_angle;
    }
  }

  robot->floor_y += robot->vy;
  robot->x += robot->vx;
}

bool collides(Robot* robot, Entity* entity, float ms) {
  Polygon* entity_poly = &entity->poly;
  PolygonIntersection m = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x1 + robot->vx, robot->floor_y + robot->vy});
  if (!m.intersects) {
    m = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x2 + robot->vx, robot->floor_y + robot->vy});
  }

  if (m.intersects) {
    float ceiling = 1;
    float floor = 0;
    float fac = 0.5;
    float vx = robot->vx;
    float vy = robot->vy;
    while (ceiling - floor > 0.01) {
      vx = robot->vx * fac;
      vy = robot->vy * fac;
      m = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x1 + robot->vx, robot->floor_y + robot->vy});
      if (!m.intersects) {
        m = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x2 + robot->vx, robot->floor_y + robot->vy});
      }
      if (!m.intersects) {
        floor = fac;
      } else {
        ceiling = fac;
      }
      fac = floor + (ceiling - floor)/2;
    }
    if (fabs(vx) <= 0.01) {
      vx = 0;
    }
    if (fabs(vy) <= 0.01) {
      vy = 0;
    }
    robot->vx = vx;
    robot->vy = vy;
    robot->collided = true;
    return true;
  }
  return false;
}

bool z(float n) {
  return n < 0.0001 && n > -0.0001;
}

bool select(Entity* entity, void* _selected_entity) {
  Entity** selected_entity = _selected_entity;
  Vec2 p = {screen_to_entity(screen_x_to_z0(window.mouse_x)), screen_to_entity(screen_y_to_z0(window.mouse_y))};
  if (polygon_contains_point(&entity->poly, p).intersects) {
    *selected_entity = entity;
    return false;
  }
  return true;
}

bool collide(Entity* entity, void* _ms) {
  float* ms = _ms;
  return !collides(&g.robot, entity, *ms);
}

bool snap(Entity* entity, void* _data) {
  for (size_t i=0; i < entity->poly.count; i++) {
    if (snap_foot_to_surface(&g.robot, entity, i)) {
      return false;
    }
  }
  return true;
}

bool test_tick(float ms) {
  mouse_viewport();

  static Entity* selected_entity;
  if (window.mouse_buttons[MOUSE_LEFT] && !selected_entity) {
    visit_entities(select, &selected_entity);
  } else if (!window.mouse_buttons[MOUSE_LEFT]) {
    selected_entity = NULL;
  }

  float x, y;
  if (selected_entity && drag_delta(&x, &y, MOUSE_LEFT)) {
    selected_entity->poly.ox += screen_to_entity(screen_to_z0(x));
    selected_entity->poly.oy += screen_to_entity(screen_to_z0(y));
    polygon_position_changed(&selected_entity->poly);
  }

  static float q = true;
  if (key_pressed(KEY_Q)) {
    q = !q;
  }
  if (key_pressed(KEY_Z)) {
    g.robot.x = -100;
    g.robot.vx = 0;
    g.robot.vy = 0;
    g.robot.floor_y = -100;
    g.robot.floor.state = false;
  }
  if (q || key_pressed_repeating(KEY_A)) {
    if (g.robot.floor.state) {
      g.robot.vy = 0;
    } else {
      float gravity = ms * 4;
      if (g.robot.vy < gravity) {
        g.robot.vy = fmin(gravity, g.robot.vy + ms / 80);
      }
    }
    float dir = window.keys[KEY_RIGHT] - (float) window.keys[KEY_LEFT];
    if (z(dir)) {
      g.robot.vx *= 0.75;
    } else {
      float target = ms * 0.08;
      if (g.robot.vx * dir < target) {
        g.robot.vx = fmin(target, g.robot.vx * dir + ms / 60) * dir;
      }
    }
    if (g.robot.floor.state && key_pressed(KEY_SPACE)) {
      g.robot.vy = -5;
      g.robot.floor.state = false;
    } else {
      if (!g.robot.floor.state) {
        g.robot.target_floor_angle = 0;
        g.robot.collided = false;
        visit_entities(collide, &ms);
        g.robot.collided && printf("collided!\n");
      }
      if (g.robot.collided || g.robot.floor.state) {
        g.robot.vy = 0;
        visit_entities(snap, NULL);
        g.robot.collided = g.robot.collided && !g.robot.floor.state;
        g.robot.floor.state && printf("snapping to floor\n");
      }
    }
    update_robot(&g.robot, ms);
  }

  begin_render();
  render_entities();
  render_robot(&g.robot);
  end_render();
  return !window.closed;
}

static Program program_test = {
  .init = test_init,
  .tick = test_tick,
};
