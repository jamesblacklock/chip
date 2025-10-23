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

void set_mouse_state(float x, float y, bool l, bool r, bool m) {
  window.mouse_x = x;
  window.mouse_y = y;
  window.mouse_buttons[MOUSE_LEFT] = l;
  window.mouse_buttons[MOUSE_RIGHT] = r;
  window.mouse_buttons[MOUSE_MIDDLE] = m;
}

void set_scroll_state(float delta_x, float delta_y) {
  window.scroll_x += delta_x;
  window.scroll_y += delta_y;
  window.scroll_x_delta = delta_x;
  window.scroll_y_delta = delta_y;
}

void window_closed() {
  printf("window closed\n");
  window.closed = true;
}
