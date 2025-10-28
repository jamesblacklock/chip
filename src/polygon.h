#ifndef POLYGON_H
#define POLYGON_H

#include <stdlib.h>

#include "main.h"

typedef struct Vec2 {
  double x;
  double y;
} Vec2;

typedef struct Manifold
{
  void* A;
  void* B;
  Vec2 contact_points[2];
  Vec2 normal;
  unsigned int contact_count;
  double penetration;
} Manifold;

typedef struct Polygon {
  SerializableObject __so;
  Vec2* points;
  size_t count;
  bool attempted_triangles;
  struct Polygon* triangles;
  size_t triangle_count;
  bool attempted_convexes;
  struct Polygon* convexes;
  size_t convex_count;
  float ox;
  float oy;
  float z;
  float angle;
  float min_x;
  float min_y;
  float max_x;
  float max_y;
  bool has_bounds;
} Polygon;

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C 
#endif

typedef struct PolygonIntersection {
  bool intersects;
  Vec2 point;
} PolygonIntersection;

EXTERN_C Polygon* partition_convex(Polygon* poly, size_t* output_count);
EXTERN_C Polygon* partition_triangles(Polygon* poly, size_t* output_count);
EXTERN_C Polygon create_polygon(float points[][2], size_t count);
EXTERN_C void free_polygons(Polygon* polys, size_t count);
EXTERN_C void free_polygon(Polygon* poly);
EXTERN_C void draw_polygon(Polygon* poly, float r, float g, float b);
EXTERN_C void draw_point(Vec2 point, float ox, float oy);
EXTERN_C bool validate_polygon(Polygon* poly);
EXTERN_C PolygonIntersection polygon_intersection(Polygon* poly1, Polygon* poly2);
EXTERN_C PolygonIntersection polygon_contains_point(Polygon* poly, Vec2 point);
EXTERN_C void polygon_geometry_changed(Polygon* poly);
EXTERN_C void find_bounds(Polygon* poly, bool refresh);

#endif
