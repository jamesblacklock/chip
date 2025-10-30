#include <stdbool.h>
#include <stdio.h>

#include "window.h"
#include "game.h"
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

void set_key_state(size_t key, bool state, bool repeat) {
  if (!repeat) {
    window.keys[key] = state;
  }
  window.keys_repeating[key] = state ? window.keys_repeating[key] + 1 : 0;
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

bool mouse_pressed(uint32_t button) {
  return !window.last_frame_mouse_buttons[button] && window.mouse_buttons[button];
}

bool key_pressed(uint32_t key) {
  return window.last_frame_keys[key] == 0 && window.keys[key];
}

bool key_pressed_repeating(uint32_t key) {
  return window.last_frame_keys[key] < window.keys_repeating[key];
}

bool drag_delta(float* x, float* y, uint32_t button) {
  static float _x[MOUSE_BUTTON_COUNT], _y[MOUSE_BUTTON_COUNT];
  if (mouse_pressed(button)) {
    _x[button] = window.mouse_x;
    _y[button] = window.mouse_y;
  }
  if (window.mouse_buttons[button]) {
    *x = window.mouse_x - _x[button];
    *y = window.mouse_y - _y[button];
    _x[button] = window.mouse_x;
    _y[button] = window.mouse_y;
    return true;
  } else {
    *x = *y = 0;
    return false;
  }
}
