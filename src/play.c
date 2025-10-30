#ifdef WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

#include <math.h>

#include "main.h"
#include "window.h"
#include "game.h"
#include "graphics.h"
#include "polygon.h"
#include "play.h"

typedef struct Foot {
  float x1;
  float x2;
  struct {
    Edge* edge;
    int32_t type;
  } collision;
} Foot;

typedef struct Floor {
  Edge* edge;
  float x;
  bool state;
} Floor;

typedef struct Robot {
  float x;
  float vx;
  float vy;
  float h;
  Foot foot;
  float floor_y;
  float floor_angle;
  float target_floor_angle;
  Floor floor;
} Robot;

struct {
  Robot robot;
  Map map;
} g;

static bool init(ExeArgs args) {
  if (access(args.play.mapfile, F_OK) != 0) {
    printf("failed to load map \"%s\"\n", args.play.mapfile);
    return false;
  }
  g.map = map_load_file(args.play.mapfile);
  g.robot = (Robot){
    .h = 60,
    .x = -100,
    .floor_y = -100,
    .foot = {.x1 = -14, .x2 = 18},
  };
  return true;
}

void robot_render(Robot* robot) {
  draw_line((LineData){
    .w = pixart_to_window(1),
    .x1 = pixart_to_window(robot->x + cos(robot->floor_angle) * robot->foot.x1),
    .y1 = pixart_to_window(robot->floor_y + sin(robot->floor_angle) * robot->foot.x1),
    .x2 = pixart_to_window(robot->x + cos(robot->floor_angle) * robot->foot.x2),
    .y2 = pixart_to_window(robot->floor_y + sin(robot->floor_angle) * robot->foot.x2),
    .r = 1, .g = 1,
  });
  draw_line((LineData){
    .w = pixart_to_window(1),
    .x1 = pixart_to_window(robot->x),
    .y1 = pixart_to_window(robot->floor_y),
    .x2 = pixart_to_window(robot->x),
    .y2 = pixart_to_window(robot->floor_y - robot->h),
    .r = 1, .g = 1,
  });
}

bool snap_foot_to_surface(Robot* robot) {
  Edge* edge = robot->floor.edge;
  float robot_x = robot->x;
  if (robot_x > edge->x2 && between(edge->next->normal, M_PI*5/4, M_PI*7/4)) {
    edge = robot->floor.edge = edge->next;
  } else if (robot_x < edge->x1 && between(edge->prev->normal, M_PI*5/4, M_PI*7/4)) {
    edge = robot->floor.edge = edge->prev;
  } else if (robot->floor.state && robot->floor.edge == edge) {
    robot_x = edge->x1 + robot->floor.x + robot->vx;
  }

  float subject_y = robot_x * edge->slope + edge->intercept;

  float foot_x1 = robot_x + cos(robot->floor_angle) * robot->foot.x1;
  float foot_x2 = robot_x + cos(robot->floor_angle) * robot->foot.x2;

  if (robot_x + robot->foot.x2 >= edge->x1 && robot_x + robot->foot.x1 < edge->x2) {
    robot->floor_y = subject_y;
    robot->target_floor_angle = edge->angle;
    robot->floor.state = true;
    robot->floor.edge = edge;
    robot->floor.x = robot_x - edge->x1;
    robot->x = robot_x;
    return true;
  }
  if (robot->floor.state && robot->floor.edge == edge) {
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

typedef struct Line {
  float x1;
  float y1;
  float x2;
  float y2;
} Line;

bool lines_intersect(Line line1, Line line2, Vec2* point) {
  float a1 = line1.y2 - line1.y1;
  float b1 = line1.x1 - line1.x2;
  float c1 = line1.y1 * (line1.x2 - line1.x1) - line1.x1 * (line1.y2 - line1.y1);
  float a2 = line2.y2 - line2.y1;
  float b2 = line2.x1 - line2.x2;
  float c2 = line2.y1 * (line2.x2 - line2.x1) - line2.x1 * (line2.y2 - line2.y1);
  float x = (b1*c2-b2*c1)/(a1*b2-a2*b1);
  float y = (c1*a2-c2*a1)/(a1*b2-a2*b1);
  if (point != NULL) {
    point->x = x;
    point->y = y;
  }
  return (
    between(x, fminf(line1.x1, line1.x2), fmaxf(line1.x1, line1.x2)) &&
    between(x, fminf(line2.x1, line2.x2), fmaxf(line2.x1, line2.x2)) &&
    between(y, fminf(line1.y1, line1.y2), fmaxf(line1.y1, line1.y2)) &&
    between(y, fminf(line2.y1, line2.y2), fmaxf(line2.y1, line2.y2))
  );
}

#define FOOT_INTERSECTS_NONE -1
#define FOOT_INTERSECTS_2 0
#define FOOT_INTERSECTS_1 1
#define FOOT_INTERSECTS_MID 2

int32_t foot_intersects_edge(Robot* robot, Edge* edge) {
  Line foot_line = {robot->x, robot->floor_y, robot->x+robot->vx, robot->floor_y+robot->vy};
  Line edge_line = {edge->x1, edge->y1, edge->x2, edge->y2};
  if (lines_intersect(foot_line, edge_line, NULL)) {
    return FOOT_INTERSECTS_MID;
  }
  foot_line = (Line){robot->x+robot->foot.x1, robot->floor_y, robot->x+robot->foot.x1+robot->vx, robot->floor_y+robot->vy};
  if (lines_intersect(foot_line, edge_line, NULL)) {
    return FOOT_INTERSECTS_1;
  }
  foot_line = (Line){robot->x+robot->foot.x2, robot->floor_y, robot->x+robot->foot.x2+robot->vx, robot->floor_y+robot->vy};
  if (lines_intersect(foot_line, edge_line, NULL)) {
    return FOOT_INTERSECTS_2;
  }
  return FOOT_INTERSECTS_NONE;
}

bool collides(Robot* robot, Polygon* poly) {
  if (robot->vy < 0) {
    return false;
  }
  int32_t type = FOOT_INTERSECTS_NONE;
  Edge* edge = NULL;
  for (size_t i=0; i < poly->count; i++) {
    if (!between(poly->edges[i].normal, M_PI*5/4, M_PI*7/4)) {
      continue;
    }
    int32_t type0 = foot_intersects_edge(robot, &poly->edges[i]);
    if (type0 > type) {
      type = type0;
      edge = &poly->edges[i];
    }
  }
  if (type == FOOT_INTERSECTS_NONE || type <= robot->foot.collision.type) {
    return false;
  }
  robot->foot.collision.edge = edge;
  robot->foot.collision.type = type;
  return true;
}

void collide() {
  for (size_t i=0; i < g.map.poly_count; i++) {
    if (collides(&g.robot, &g.map.polys[i]) && g.robot.foot.collision.type < FOOT_INTERSECTS_MID) {
      break;
    }
  }
}

static void viewport_box_follow() {
  float chip_x = pixart_to_window(g.robot.x);
  float chip_y = pixart_to_window(g.robot.floor_y);
  float vx = viewport.x;
  float vy = viewport.y;
  if (viewport.x < chip_x - window.width/5) {
    vx = chip_x - window.width/5;
  } else if (viewport.x > chip_x + window.width/5) {
    vx = chip_x + window.width/5;
  }
  if (viewport.y < chip_y - window.height/5) {
    vy = chip_y - window.height/5;
  } else if (viewport.y > chip_y + window.height/5) {
    vy = chip_y + window.height/5;
  }
  set_view_coords(vx, vy, NEUTRAL_Z_DIST);
}

bool tick(float ms) {
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
      g.robot.foot.collision.type = FOOT_INTERSECTS_NONE;
      collide();
      if (g.robot.foot.collision.type > FOOT_INTERSECTS_NONE) {
        g.robot.floor.edge = g.robot.foot.collision.edge;
        snap_foot_to_surface(&g.robot);
      }
    }
    if (g.robot.floor.state) {
      g.robot.vy = 0;
      snap_foot_to_surface(&g.robot);
    }
  }
  update_robot(&g.robot, ms);
  viewport_box_follow();

  begin_render();
  map_render(&g.map);
  robot_render(&g.robot);
  end_render();
  return !window.closed;
}

Program program_play = {
  .init = init,
  .tick = tick,
};
