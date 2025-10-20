#ifndef POLYGON_H
#define POLYGON_H

#include <stdlib.h>

#include "main.h"

typedef struct Point {
  float x;
  float y;
} Point;

typedef struct Polygon {
  SerializableObject __so;
  Point* points;
  size_t count;
  struct Polygon* triangles;
  size_t triangle_count;
} Polygon;

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C 
#endif

EXTERN_C Polygon* partition_convex(Polygon* poly, size_t* output_count);
EXTERN_C Polygon* partition_triangles(Polygon* poly, size_t* output_count);
EXTERN_C Polygon create_polygon(float points[][2], size_t count);
EXTERN_C void free_polygons(Polygon* polys, size_t count);
EXTERN_C void free_polygon(Polygon* poly);
EXTERN_C void draw_polygon(Polygon* poly);
EXTERN_C bool validate_polygon(Polygon* poly);

#endif
