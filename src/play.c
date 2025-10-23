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
  init_entities();
  if (access(args.play.mapfile, F_OK) != 0) {
    printf("failed to load map \"%s\"\n", args.play.mapfile);
    return false;
  }
  g_map = load_map_file(args.play.mapfile);
  chip = create_entity((Entity){
    .x = 0,
    .y = 0,
    .w = 40,
    .h = 60,
    .color = {1, 1, 1},
  });
  attach_body(chip, true);
  // set_view_coords(200, -300, NEUTRAL_Z_DIST);
  return true;
}

static bool tick(float ms) {
  if (key_pressed(KEY_SPACE)) {
    b2Body_ApplyLinearImpulseToCenter(chip->body, (b2Vec2){0, -500000}, true);
  }
  if (window.keys[KEY_RIGHT]) {
    b2Body_ApplyLinearImpulseToCenter(chip->body, (b2Vec2){10000}, true);
  }
  if (window.keys[KEY_LEFT]) {
    b2Body_ApplyLinearImpulseToCenter(chip->body, (b2Vec2){-10000}, true);
  }

  update_entities();
  begin_render();
  render_entities();
  end_render();
  return !window.closed;
}

Program program_play = {
  .init = init,
  .tick = tick,
};
