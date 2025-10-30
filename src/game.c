#include "game.h"
#include "window.h"
#include "graphics.h"
#include "main.h"

GameGlobals game_globals;

void update_pixart_unit() {
  game_globals.pixart_unit = fmax(1, round(sqrt(window.width * window.height) / 548.63467));
}

void draw_map_poly(Polygon* poly) {
  Polygon* tris = polygon_tris(poly);
  if (!tris) {
    return;
  }

  for (size_t i=0; i < poly->ntris; i++) {
    draw_triangle((TriangleData){
      .ox = pixart_to_window(poly->x),
      .oy = pixart_to_window(poly->y),
      .x1 = pixart_to_window(tris[i].points[0].x),
      .y1 = pixart_to_window(tris[i].points[0].y),
      .x2 = pixart_to_window(tris[i].points[1].x),
      .y2 = pixart_to_window(tris[i].points[1].y),
      .x3 = pixart_to_window(tris[i].points[2].x),
      .y3 = pixart_to_window(tris[i].points[2].y),
      .z = pixart_to_window(poly->z),
      .b = 1, .g = 0.1, .reuse_attrs = i > 0,
      // .angle = poly->angle,
    });
  }
  
  for (size_t i=0; i < poly->count; i++) {
    Edge* edge = &poly->edges[i];
    bool floor = between(edge->normal, M_PI*5/4, M_PI*7/4);
    if (!floor) {
      draw_line((LineData){
        .x1 = pixart_to_window(edge->x1),
        .y1 = pixart_to_window(edge->y1),
        .x2 = pixart_to_window(edge->x2),
        .y2 = pixart_to_window(edge->y2),
        .w = pixart_to_window(1),
        .r = 1, .g = 0.1,
      });
    }
    // Vec2 p1 = poly->points[i];
    // Vec2 p2 = poly->points[(i+1) % poly->count];
    // draw_point({
    //   .x = edge->midpoint.x,
    //   .y = edge->midpoint.y,
    // }, 0, 0);
    // draw_line((LineData){
    //   .x1 = pixart_to_window(edge->midpoint.x),
    //   .y1 = pixart_to_window(edge->midpoint.y),
    //   .x2 = pixart_to_window(edge->midpoint.x + cos(edge->normal) * 20),
    //   .y2 = pixart_to_window(edge->midpoint.y + sin(edge->normal) * 20),
    //   .w = pixart_to_window(1),
    //   .r = 1, .g = between(edge->normal, M_PI*5/4, M_PI*7/4) ? 1.f : 0.f,
    // });
  }
}

void map_render(Map* map) {
  for (size_t i=0; i < map->poly_count; i++) {
    draw_map_poly(&map->polys[i]);
  }
}

bool map_save_file(const char* filename, Map* map) {
  FILE* fp = fopen(filename, "wb");
  for (size_t i=0; i < map->poly_count; i++) {
    write_object(&map->polys[i], fp);
  }
  write_eof(fp);
  fclose(fp);
  return true;
}

#define MAP_MAX_POLYGONS 6000

Map map_load_file(const char* filename) {
  FILE* fp = fopen(filename, "rb");
  Polygon* poly = (Polygon*) read_object(fp);

  Polygon* polys = malloc(sizeof(Polygon) * MAP_MAX_POLYGONS);
  size_t poly_count = 0;
  while (poly != NULL) {
    if (poly->__so.object_type != OBJECT_TYPE_POLYGON) {
      printf("encountered unexpected object: %d", poly->__so.object_type);
      continue;
    }
    polys[poly_count++] = *poly;
    // Color color = { rand() / (float) RAND_MAX, rand() / (float) RAND_MAX, rand() / (float) RAND_MAX };
    poly = (Polygon*) read_object(fp);
  }
  fclose(fp);
  polys = realloc(polys, sizeof(Polygon) * poly_count);
  return (Map){ .polys = polys, .poly_count = poly_count };
}

void map_free(Map* map) {
  if (!map->polys) return;
  for (size_t i=0; i < map->poly_count; i++) {
    polygon_free(&map->polys[i]);
  }
}

// void draw_point(Vec2 point, float ox, float oy) {
//   draw_quad((QuadData){
//     .x = pixart_to_window(point.x + ox),
//     .y = pixart_to_window(point.y + oy),
//     .w = 8, .h = 8, .b = 1, .g = 1,
//   });
//   draw_quad((QuadData){
//     .x = pixart_to_window(point.x + ox),
//     .y = pixart_to_window(point.y + oy),
//     .w = 4, .h = 4, .r = 1, .b = 1,
//   });
// }

// void draw_points(Polygon* poly) {
//   for (size_t i=0; i < poly->count; i++) {
//     draw_point(poly->points[i], poly->ox, poly->oy);
//   }
// }
