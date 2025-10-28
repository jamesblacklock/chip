#ifdef WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif
#include <string.h>

// #include "main.h"
#include "graphics.h"
#include "window.h"
#include "entity.h"

static Map g_map;
static Entity* chip;

static bool init(ExeArgs args) {
  // init_entities();
  // if (access(args.play.mapfile, F_OK) != 0) {
  //   printf("failed to load map \"%s\"\n", args.play.mapfile);
  //   return false;
  // }
  // g_map = load_map_file(args.play.mapfile);
  // chip = create_entity((Entity){
  //   .x = 0,
  //   .y = 0,
  //   .w = 40,
  //   .h = 60,
  //   .color = {1, 1, 1},
  // });
  // attach_body(chip, true);
  // set_view_coords(entity_to_screen(chip->x), entity_to_screen(chip->y), NEUTRAL_Z_DIST);
  return true;
}

static void viewport_lock_follow() {
  // set_view_coords(entity_to_screen(chip->x), entity_to_screen(chip->y), NEUTRAL_Z_DIST);
}
static void viewport_smooth_follow(float ms) {
  // static float vvx = 0;
  // static float vvy = 0;
  // float vdx = viewport.x < entity_to_screen(chip->x) ? 1 : viewport.x > entity_to_screen(chip->x) ? -1 : 0;
  // vvx = fmin(fabsf(vvx) + ms / 50, fabsf(entity_to_screen(chip->x) - viewport.x)/30);
  // if (vvx < 0.1) vvx = 0;
  // vvx *= vdx;
  // float vdy = viewport.y < entity_to_screen(chip->y) ? 1 : viewport.y > entity_to_screen(chip->y) ? -1 : 0;
  // vvy = fmin(fabsf(vvy) + ms / 50, fabsf(entity_to_screen(chip->y) - viewport.y)/30);
  // if (vvy < 0.1) vvy = 0;
  // vvy *= vdy;
  // set_view_coords(viewport.x + vvx, viewport.y + vvy, NEUTRAL_Z_DIST);
}
static void viewport_box_follow() {
  // float chip_x = entity_to_screen(chip->x);
  // float chip_y = entity_to_screen(chip->y);
  // float vx = viewport.x;
  // float vy = viewport.y;
  // if (viewport.x < chip_x - window.width/5) {
  //   vx = chip_x - window.width/5;
  // } else if (viewport.x > chip_x + window.width/5) {
  //   vx = chip_x + window.width/5;
  // }
  // if (viewport.y < chip_y - window.height/5) {
  //   vy = chip_y - window.height/5;
  // } else if (viewport.y > chip_y + window.height/5) {
  //   vy = chip_y + window.height/5;
  // }
  // set_view_coords(vx, vy, NEUTRAL_Z_DIST);
}

static bool tick(float ms) {

  // if (key_pressed(KEY_SPACE)) {
  //   b2Body_ApplyLinearImpulseToCenter(chip->body, (b2Vec2){0, -500000}, true);
  // }
  // if (window.keys[KEY_RIGHT]) {
  //   b2Body_ApplyLinearImpulseToCenter(chip->body, (b2Vec2){10000}, true);
  // }
  // if (window.keys[KEY_LEFT]) {
  //   b2Body_ApplyLinearImpulseToCenter(chip->body, (b2Vec2){-10000}, true);
  // }
  // if (window.keys[KEY_LSHIFT]) {
  //   // b2Body_SetAngularVelocity()
  // }

  update_entities(ms);
  begin_render();
  viewport_box_follow();
  render_entities();
  end_render();
  return !window.closed;
}

Program program_play = {
  .init = init,
  .tick = tick,
};
