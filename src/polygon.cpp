#include <polypartition.h>
#include <SP2C/SPC_Shapes.h>
#include <SP2C/SPC_Collision.h>
#include <clipper2/clipper.h>
#include <list>
#include <vector>
#include <stdlib.h>
#include <cglm/cglm.h>

#include "main.h"
#include "polygon.h"
#include "graphics.h"
#include "entity.h"

Polygon* partition(Polygon* in_poly, size_t* output_count, bool reverse, void (*algo)(TPPLPartition*, TPPLPoly*, std::list<TPPLPoly>*)) {
  TPPLPoly poly;
  poly.Init(in_poly->count);
  if (reverse) {
    for (size_t i=0; i < in_poly->count; i++) {
      poly[i].x = in_poly->points[in_poly->count-i-1].x;
      poly[i].y = in_poly->points[in_poly->count-i-1].y;
    }
  } else {
    for (size_t i=0; i < in_poly->count; i++) {
      poly[i].x = in_poly->points[i].x;
      poly[i].y = in_poly->points[i].y;
    }
  }
  TPPLPartition part;
  std::list<TPPLPoly> result;
  algo(&part, &poly, &result);
  if (result.size() == 0 && !reverse) {
    return partition(in_poly, output_count, true, algo);
  }
  *output_count = result.size();
  if (result.size() == 0) {
    return NULL;
  }
  Polygon* output = (Polygon*) malloc(sizeof(Polygon) * result.size());
  Polygon* cur = output;
  for (auto it=result.begin(); it != result.end(); it++) {
    cur->triangle_count = 0;
    cur->triangles = NULL;
    cur->has_bounds = false;
    cur->count = it->GetNumPoints();
    cur->points = (Vec2*) malloc(sizeof(Vec2) * cur->count);
    auto points = it->GetPoints();
    for (size_t i=0; i < cur->count; i++) {
      cur->points[i].x = points[i].x;
      cur->points[i].y = points[i].y;
    }
    cur++;
  }
  return output;
}

extern "C" {

Polygon create_polygon(Vec2* points, size_t count) {
  Polygon p = {
    .__so.object_type = OBJECT_TYPE_POLYGON,
    .count = count,
    .points = (Vec2*) malloc(sizeof(Vec2) * count),
    .edges = (Edge*) malloc(sizeof(Edge) * count),
    .triangle_count = 0,
    .triangles = NULL,
    .attempted_triangles = false,
    .has_bounds = false,
  };
  if (points_are_clockwise(points, count)) {
    for (size_t i=0; i < count; i++) {
      p.points[i] = points[i];
      p.points[i] = points[i];
    }
  } else {
    for (size_t i=0; i < count; i++) {
      p.points[i] = points[count - i - 1];
      p.points[i] = points[count - i - 1];
    }
  }
  polygon_position_changed(&p);

  return p;
}

bool validate_polygon(Polygon* poly) {
  if (!poly->attempted_triangles) {
    poly->triangles = partition_triangles(poly, &poly->triangle_count);
    poly->attempted_triangles = true;
  }
  return poly->triangles != NULL;
}

void free_polygon(Polygon* poly) {
  free(poly->points);
  free_polygons(poly->triangles, poly->triangle_count);
}

void free_polygons(Polygon* polys, size_t count) {
  if (polys == NULL || count == 0) {
    return;
  }
  for (size_t i=0; i < count; i++) {
    free_polygon(&polys[i]);
  }
  free(polys);
}

Polygon* partition_triangles(Polygon* poly, size_t* output_count) {
  return partition(poly, output_count, false, [] (TPPLPartition* part, TPPLPoly* poly, std::list<TPPLPoly>* res) { part->Triangulate_OPT(poly, res); });
}

void draw_point(Vec2 point, float ox, float oy) {
  draw_quad((QuadData){
    .x = entity_to_screen(point.x + ox),
    .y = entity_to_screen(point.y + oy),
    .w = 8, .h = 8, .b = 1, .g = 1,
  });
  draw_quad((QuadData){
    .x = entity_to_screen(point.x + ox),
    .y = entity_to_screen(point.y + oy),
    .w = 4, .h = 4, .r = 1, .b = 1,
  });
}

void draw_points(Polygon* poly) {
  for (size_t i=0; i < poly->count; i++) {
    draw_point(poly->points[i], poly->ox, poly->oy);
  }
}

float fclamp(float n, float min, float max) {
  return n > max ? max : n < min ? min : n;
}

bool between(float n, float min, float max) {
  return n > max ? false : n < min ? false : true;
}

void draw_polygon(Polygon* poly, float r, float g, float b) {
  if (!validate_polygon(poly)) {
    fprintf(stderr, "couldn't validate polygon\n");
    return;
  }

  for (size_t i=0; i < poly->triangle_count; i++) {
    draw_triangle((TriangleData){
      .ox = entity_to_screen(poly->ox),
      .oy = entity_to_screen(poly->oy),
      .x1 = entity_to_screen(poly->triangles[i].points[0].x),
      .y1 = entity_to_screen(poly->triangles[i].points[0].y),
      .x2 = entity_to_screen(poly->triangles[i].points[1].x),
      .y2 = entity_to_screen(poly->triangles[i].points[1].y),
      .x3 = entity_to_screen(poly->triangles[i].points[2].x),
      .y3 = entity_to_screen(poly->triangles[i].points[2].y),
      .z = entity_to_screen(poly->z),
      .r = r, .g = g, .b = b,
      .reuse_attrs = i > 0,
      .angle = poly->angle,
    });
  }
  
  for (size_t i=0; i < poly->count; i++) {
    Edge* edge = &poly->edges[i];
    bool floor = between(edge->normal, M_PI*5/4, M_PI*7/4);
    draw_line((LineData){
      .x1 = entity_to_screen(edge->x1),
      .y1 = entity_to_screen(edge->y1),
      .x2 = entity_to_screen(edge->x2),
      .y2 = entity_to_screen(edge->y2),
      .w = entity_to_screen(2),
      .r = floor ? 0.f : 1.f,
      .g = floor ? 1.f : 0.f,
    });
    // Vec2 p1 = poly->points[i];
    // Vec2 p2 = poly->points[(i+1) % poly->count];
    // draw_point({
    //   .x = edge->midpoint.x,
    //   .y = edge->midpoint.y,
    // }, 0, 0);
    // draw_line((LineData){
    //   .x1 = entity_to_screen(edge->midpoint.x),
    //   .y1 = entity_to_screen(edge->midpoint.y),
    //   .x2 = entity_to_screen(edge->midpoint.x + cos(edge->normal) * 20),
    //   .y2 = entity_to_screen(edge->midpoint.y + sin(edge->normal) * 20),
    //   .w = entity_to_screen(1),
    //   .r = 1, .g = between(edge->normal, M_PI*5/4, M_PI*7/4) ? 1.f : 0.f,
    // });
  }
}

Edge edge_info(Polygon* poly, size_t edge) {
  Vec2 p1 = poly->points[edge];
  Vec2 p2 = poly->points[(edge+1) % poly->count];
  float x1 = p1.x + poly->ox;
  float y1 = p1.y + poly->oy;
  float x2 = p2.x + poly->ox;
  float y2 = p2.y + poly->oy;
  float rise = y2-y1;
  float run = x2-x1;
  float slope = rise/run;
  float angle = atan(slope);
  if (p1.x > p2.x) angle += M_PI;
  if (angle < 0) angle += M_PI*2;
  float normal = angle - M_PI_2;
  if (normal < 0) normal += M_PI*2;
  float intercept = y1 - slope * x1;
  float midpoint_x = poly->ox + p1.x + (p2.x - p1.x)/2;
  float midpoint_y = isinf(slope) ? poly->oy + p1.y + (p2.y - p1.y)/2 : slope * midpoint_x + intercept;
  return {
    .x1 = x1,
    .y1 = y1,
    .x2 = x2,
    .y2 = y2,
    .slope = slope,
    .intercept = intercept,
    .midpoint = {midpoint_x, midpoint_y},
    .angle = angle,
    .normal = normal,
  };
}

void polygon_geometry_changed(Polygon* poly) {
  free_polygons(poly->triangles, poly->triangle_count);
  poly->triangles = NULL;
  poly->triangle_count = 0;
  poly->attempted_triangles = false;
  poly->has_bounds = false;
  polygon_position_changed(poly);
}

void polygon_position_changed(Polygon* poly) {
  for (size_t i=0; i < poly->count; i++) {
    Vec2 p1 = poly->points[i];
    Vec2 p2 = poly->points[(i+1) % poly->count];
    poly->edges[i] = edge_info(poly, i);
  }
}

void find_bounds(Polygon* poly, bool refresh) {
  if (!refresh && poly->has_bounds) {
    return;
  }
  float min_x = INFINITY, min_y = INFINITY, max_x = -INFINITY, max_y = -INFINITY;
  for (size_t i=0; i < poly->count; i++) {
    min_x = fmin(min_x, poly->points[i].x + poly->ox);
    min_y = fmin(min_y, poly->points[i].y + poly->oy);
    max_x = fmax(max_x, poly->points[i].x + poly->ox);
    max_y = fmax(max_y, poly->points[i].y + poly->oy);
  }
  poly->min_x = min_x;
  poly->min_y = min_y;
  poly->max_x = max_x;
  poly->max_y = max_y;
}

PolygonIntersection polygon_contains_point(Polygon* poly, Vec2 point) {
  PolygonIntersection pi = {.intersects = false, .point = point};
  float f;
  for (size_t i=0; i < poly->count; i++) {
    Vec2 vc = poly->points[i];
    Vec2 vn = poly->points[(i + 1) % poly->count];
    vc.x = vc.x + poly->ox;
    vc.y = vc.y + poly->oy;
    vn.x = vn.x + poly->ox;
    vn.y = vn.y + poly->oy;
    if ((vc.y > point.y) != (vn.y > point.y) && (point.x < (vn.x-vc.x)*(point.y-vc.y) / (vn.y-vc.y)+vc.x)) {
      pi.intersects = !pi.intersects;
    }
  }
  return pi;
}

PolygonIntersection polygon_intersection_impl(Polygon* poly1, Polygon* poly2, float ox, float oy) {
  for (size_t i=0; i < poly1->count; i++) {
    Vec2 p = poly1->points[i];
    p.x += ox;
    p.y += oy;
    PolygonIntersection pi = polygon_contains_point(poly2, p);
    if (pi.intersects) {
      return pi;
    }
  }
  return (PolygonIntersection){.intersects = false};
}

PolygonIntersection polygon_intersection(Polygon* poly1, Polygon* poly2) {
  PolygonIntersection pi = polygon_intersection_impl(poly1, poly2, poly1->ox, poly1->oy);
  if (pi.intersects) {
    return pi;
  }
  return polygon_intersection_impl(poly2, poly1, poly2->ox, poly2->oy);
}

float cross_product(Vec2 a, Vec2 b) {
  return (a.x*b.y) - (a.y*b.x);
}

bool points_are_clockwise(Vec2* points, size_t count) {
  size_t a = 0;
  for (size_t i=1; i < count; i++) {
    if (points[i].y < points[a].y || (points[i].y == points[a].y && points[i].x > points[a].x)) {
      a = i;
    }
  }
  size_t b = a == 0 ? count - 1 : a - 1;
  size_t c = (a + 1) % count;
  Vec2 ab = {points[b].x-points[a].x, points[b].y-points[a].y};
  Vec2 ac = {points[c].x-points[a].x, points[c].y-points[a].y};
  return cross_product(ab, ac) < 0;
}








void rotate_polygon(Polygon* poly, float rad) {
  mat3 mat;
  glm_mat3_identity(mat);
  glm_rotate2d(mat, rad);
  for (size_t i=0; i < poly->count; i++) {
    vec3 p = {(float) poly->points[i].x, (float) poly->points[i].y, 0};
    glm_mat3_mulv(mat, p, p);
    poly->points[i].x = p[0];
    poly->points[i].y = p[1];
  }
  polygon_geometry_changed(poly);
}

}
