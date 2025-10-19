#include <polypartition.h>
#include <list>
#include <stdlib.h>

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
    cur->count = it->GetNumPoints();
    cur->points = (Point*) malloc(sizeof(Point) * cur->count);
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
    .count = count,
    .points = (Point*) malloc(sizeof(Point) * count),
    .triangle_count = 0,
    .triangles = NULL,
  };
  for (size_t i=0; i < count; i++) {
    p.points[i].x = points[i][0];
    p.points[i].y = points[i][1];
  }
  return p;
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

Polygon* partition_convex(Polygon* poly, size_t* output_count) {
  return partition(poly, output_count, false, [] (TPPLPartition* part, TPPLPoly* poly, std::list<TPPLPoly>* res) { part->ConvexPartition_OPT(poly, res); });
}

Polygon* partition_triangles(Polygon* poly, size_t* output_count) {
  return partition(poly, output_count, false, [] (TPPLPartition* part, TPPLPoly* poly, std::list<TPPLPoly>* res) { part->Triangulate_OPT(poly, res); });
}

void draw_polygon(Polygon* poly) {
  if (poly->triangles == NULL) {
    poly->triangles = partition_triangles(poly, &poly->triangle_count);
  }

  for (size_t i=0; i < poly->triangle_count; i++) {
    float p1[2], p2[2], p3[2];
    screen_to_world(entity_to_screen(poly->triangles[i].points[0].x), entity_to_screen(poly->triangles[i].points[0].y), p1);
    screen_to_world(entity_to_screen(poly->triangles[i].points[1].x), entity_to_screen(poly->triangles[i].points[1].y), p2);
    screen_to_world(entity_to_screen(poly->triangles[i].points[2].x), entity_to_screen(poly->triangles[i].points[2].y), p3);
    draw_triangle((TriangleData){
      .x1 = p1[0],
      .y1 = p1[1],
      .x2 = p2[0],
      .y2 = p2[1],
      .x3 = p3[0],
      .y3 = p3[1],
      .g = 1,
      .b = 1,
    });
  }
}

}
