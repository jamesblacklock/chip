#include <stdbool.h>
#include <stdio.h>

#include "window.h"
#include "entity.h"
#include "graphics.h"

Window window = {};

void init_window(uint32_t width, uint32_t height) {
  window_resized(width, height);
}

void window_resized(uint32_t width, uint32_t height) {
  window.width = width;
  window.height = height;
  update_pixart_unit();
  surface_dimensions_changed(width, height);
}

void set_key_state(size_t key, bool state) {
  window.keys[key] = state;
}

void set_mouse_state(float x, float y, bool l, bool r) {
  window.mouse_x = x;
  window.mouse_y = y;
  window.mouse_left = l;
  window.mouse_right = r;
}

void window_closed() {
  printf("window closed\n");
  window.closed = true;
}
