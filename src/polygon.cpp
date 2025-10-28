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

Polygon create_polygon(float points[][2], size_t count) {
  Polygon p = {
    .__so.object_type = OBJECT_TYPE_POLYGON,
    .count = count,
    .points = (Vec2*) malloc(sizeof(Vec2) * count),
    .triangle_count = 0,
    .triangles = NULL,
    .attempted_triangles = false,
    .attempted_convexes = false,
    .has_bounds = false,
  };
  for (size_t i=0; i < count; i++) {
    p.points[i].x = points[i][0];
    p.points[i].y = points[i][1];
  }
  return p;
}

Polygon create_polygon_d(Vec2* points, size_t count) {
  Polygon p = {
    .__so.object_type = OBJECT_TYPE_POLYGON,
    .count = count,
    .points = (Vec2*) malloc(sizeof(Vec2) * count),
    .triangle_count = 0,
    .triangles = NULL,
    .attempted_triangles = false,
    .attempted_convexes = false,
    .has_bounds = false,
  };
  memcpy(p.points, points, sizeof(Vec2) * count);
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

Polygon* partition_convex(Polygon* poly, size_t* _output_count) {
  size_t output_count;
  Polygon* result = partition(poly, &output_count, false, [] (TPPLPartition* part, TPPLPoly* poly, std::list<TPPLPoly>* res) { part->ConvexPartition_OPT(poly, res); });
  if (result == NULL) {
    result = partition(poly, &output_count, false, [] (TPPLPartition* part, TPPLPoly* poly, std::list<TPPLPoly>* res) { part->ConvexPartition_HM(poly, res); });
  }
  poly->convexes = result;
  poly->convex_count = output_count;
  if (_output_count) {
    *_output_count = output_count;
  }
  return result;
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

void draw_polygon2(Polygon* poly, float r, float g, float b, float angle) {
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
      .angle = angle + poly->angle,
    });
  }
}

void draw_polygon(Polygon* poly, float r, float g, float b) {
  draw_polygon2(poly, r, g, b, 0);
}

void polygon_geometry_changed(Polygon* poly) {
  free_polygons(poly->triangles, poly->triangle_count);
  poly->triangles = NULL;
  poly->triangle_count = 0;
  poly->attempted_triangles = false;
  poly->has_bounds = false;
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
      // printf("%f\n", atan((vn.y-vc.y)/(vn.x-vc.x)) / M_PI / 2 * 360);
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
