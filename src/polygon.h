#ifndef POLYGON_H
#define POLYGON_H

#include <stdlib.h>

#include "main.h"

typedef struct Manifold
{
  void* A;
  void* B;
  Vec2 contact_points[2];
  Vec2 normal;
  unsigned int contact_count;
  double penetration;
} Manifold;

typedef struct Edge {
  float x1;
  float y1;
  float x2;
  float y2;
  float slope;
  float intercept;
  Vec2 midpoint;
  float angle;
  float normal;
  struct Edge* prev;
  struct Edge* next;
} Edge;

typedef struct Polygon {
  SerializableObject __so;
  Vec2* points;
  Edge* edges;
  size_t count;
  float x;
  float y;
  float z;
  // float angle;
  // float min_x;
  // float min_y;
  // float max_x;
  // float max_y;
  // bool has_bounds;
  Color color;
  bool _computed_tris;
  struct Polygon* _tris;
  size_t ntris;
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

EXTERN_C Polygon polygon_new(Vec2* points, size_t count);
EXTERN_C void polygon_free(Polygon* poly);
EXTERN_C Polygon* polygon_tris(Polygon* poly);
EXTERN_C bool polygon_contains_point(Polygon* poly, Vec2 point);
EXTERN_C void polygon_geometry_changed(Polygon* poly);
EXTERN_C void polygon_position_changed(Polygon* poly);

#endif
