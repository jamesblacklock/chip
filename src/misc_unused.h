// void draw_line_intersection() {
//   Line line1 = {-82.000, 8.864, -82.000, 14.270};
//   Line line2 = {-88.000, 11.950, 24.073, 11.950};
//   Vec2 p;
//   bool draw_point = lines_intersect(line1, line2, &p);

//   begin_render();
//   draw_line((LineData){
//     .x1=line1.x1,
//     .y1=line1.y1,
//     .x2=line1.x2,
//     .y2=line1.y2,
//     .r=1,.w=2,
//   });
//   draw_line((LineData){
//     .x1=line2.x1,
//     .y1=line2.y1,
//     .x2=line2.x2,
//     .y2=line2.y2,
//     .b=1,.w=2,
//   });
//   if (draw_point) {
//     draw_quad((QuadData){.x=p.x,.y=p.y,.w=4,.h=4,.r=1,.g=1});
//   }
//   end_render();
// }


// bool collides(Robot* robot, Entity* entity) {
//   bool intersects = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x1 + robot->vx, robot->floor_y + robot->vy});
//   if (!intersects) {
//     intersects = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x2 + robot->vx, robot->floor_y + robot->vy});
//   }

//   if (intersects) {
//     float ceiling = 1;
//     float floor = 0;
//     float fac = 0.5;
//     float vx = robot->vx;
//     float vy = robot->vy;
//     while (ceiling - floor > 0.01) {
//       vx = robot->vx * fac;
//       vy = robot->vy * fac;
//       intersects = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x1 + robot->vx, robot->floor_y + robot->vy});
//       if (!intersects) {
//         intersects = polygon_contains_point(entity_poly, (Vec2){robot->x + robot->foot.x2 + robot->vx, robot->floor_y + robot->vy});
//       }
//       if (!intersects) {
//         floor = fac;
//       } else {
//         ceiling = fac;
//       }
//       fac = floor + (ceiling - floor)/2;
//     }
//     if (fabs(vx) <= 0.01) {
//       vx = 0;
//     }
//     if (fabs(vy) <= 0.01) {
//       vy = 0;
//     }
//     robot->vx = vx;
//     robot->vy = vy;
//     robot->collided = true;
//     return true;
//   }
//   return false;
// }



// #define FREELIST_DATATYPE Entity
// #include "freelist.h"

// void init_entities() {
//   Entity_FreelistInit(1000);
//   update_pixart_unit();
// }

// void visit_entities(bool (*visitor)(Entity*, void*), void* data) {
//   size_t count = 0;
//   for (size_t i=1; i <= Entity_FreelistSize; i++) {
//     size_t r = Entity_FreelistSize - i;
//     if (Entity_FreelistHeap[r].free) {
//       continue;
//     }
//     if (!visitor(&Entity_FreelistHeap[r].item, data)) {
//       break;
//     }
//   }
// }

// Entity* create_entity(Entity new_entity) {
//   Entity* entity = Entity_FreelistAlloc();
//   *entity = new_entity;
//   entity->enabled = true;
//   return entity;
// }

// typedef struct SubjectEntityInfo {
//   float vx;
//   float vy;
//   Entity* subject;
// } SubjectEntityInfo;

// bool collide_entities(Entity* entity, void* _info) {
//   SubjectEntityInfo* info = _info;
//   if (!entity->poly.count) {
//     printf("we need a polygon for collision!!\n");
//     return true;
//   }
//   if (entity == info->subject) {
//     return true;
//   }

//   Polygon* entity_poly = &entity->poly;
//   Polygon* subject_poly = &info->subject->poly;
//   float ox = subject_poly->ox;
//   float oy = subject_poly->oy;
//   find_bounds(subject_poly, false);
//   float p = 1 - 1 / (subject_poly->max_y - subject_poly->min_y);
//   subject_poly->ox = info->subject->poly.ox + info->vx;
//   subject_poly->oy = info->subject->poly.oy + info->vy;
//   PolygonIntersection m = polygon_intersection(entity_poly, subject_poly);

//   if (m.intersects) {
//     float ceiling = 1;
//     float floor = 0;
//     float fac = 0.5;
//     float vx = info->vx;
//     float vy = info->vy;
//     while (ceiling - floor > 0.01) {
//       subject_poly->ox -= vx;
//       subject_poly->oy -= vy;
//       vx = info->vx * fac;
//       vy = info->vy * fac;
//       subject_poly->ox += vx;
//       subject_poly->oy += vy;
//       m = polygon_intersection(entity_poly, subject_poly);
//       info->subject->collision.pi = m;
//       if (!m.intersects) {
//         floor = fac;
//       } else {
//         ceiling = fac;
//       }
//       fac = floor + (ceiling - floor)/2;
//     }
//     info->vx = vx;
//     info->vy = vy;
//   }
//   subject_poly->ox = ox;
//   subject_poly->oy = oy;
//   return true;
// }

// void move_entity(Entity* entity, float x, float y) {
//   SubjectEntityInfo info = { .subject = entity, .vx = x, .vy = y };

//   if (!entity->poly.count) {
//     printf("we need a polygon for collision!!\n");
//     return;
//   }
//   if (fabs(x) < 0.01) {
//     x = 0;
//     if (fabs(y) < 0.01 && !window.keys[KEY_SPACE]) {
//       return;
//     }
//   } else if (fabs(y) < 0.01) {
//     y = 0;
//   }

//   entity->collision.pi.intersects = false;
//   visit_entities(collide_entities, &info);

//   if (fabs(info.vx) >= 0.25) {
//     entity->poly.ox += info.vx;
//   }
//   if (fabs(info.vy) >= 0.25) {
//     entity->poly.oy += info.vy;
//   }
// }
