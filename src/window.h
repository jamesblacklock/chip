#ifndef WINDOW_H
#define WINDOW_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define KEY_TAB 9
#define KEY_ENTER 13
#define KEY_ESC 27
#define KEY_SPACE 32
#define KEY_COMMA 44
#define KEY_MINUS 45
#define KEY_DOT 46
#define KEY_SLASH 47
#define KEY_0 48
#define KEY_1 49
#define KEY_2 50
#define KEY_3 51
#define KEY_4 52
#define KEY_5 53
#define KEY_6 54
#define KEY_7 55
#define KEY_8 56
#define KEY_9 57
#define KEY_EQUAL 61
#define KEY_A 65
#define KEY_B 66
#define KEY_C 67
#define KEY_D 68
#define KEY_E 69
#define KEY_F 70
#define KEY_G 71
#define KEY_H 72
#define KEY_I 73
#define KEY_J 74
#define KEY_K 75
#define KEY_L 76
#define KEY_M 77
#define KEY_N 78
#define KEY_O 79
#define KEY_P 80
#define KEY_Q 81
#define KEY_R 82
#define KEY_S 83
#define KEY_T 84
#define KEY_U 85
#define KEY_V 86
#define KEY_W 87
#define KEY_X 88
#define KEY_Y 89
#define KEY_Z 90
#define KEY_LBRACK 91
#define KEY_BACKSLASH 92
#define KEY_RBRACK 93
#define KEY_BACKTICK 96
#define KEY_BSP 127

#define KEY_LSHIFT 200
#define KEY_RSHIFT 201
#define KEY_LCTRL 202
#define KEY_RCTRL 203
#define KEY_LALT 204
#define KEY_RALT 205
#define KEY_LMETA 207
#define KEY_RMETA 208
#define KEY_LEFT 209
#define KEY_RIGHT 210
#define KEY_DOWN 211
#define KEY_UP 212

#define KEY_COUNT 213

#define MOUSE_LEFT 0
#define MOUSE_RIGHT 1
#define MOUSE_MIDDLE 2
#define MOUSE_BUTTON_COUNT 3

void set_key_state(size_t key, bool state);
void set_mouse_state(float x, float y, bool l, bool r, bool m);
void set_scroll_state(float x, float y);
void init_window(uint32_t width, uint32_t height);
void window_closed();
void window_resized(uint32_t width, uint32_t height);

typedef struct Window {
  bool keys[KEY_COUNT];
  bool mouse_buttons[MOUSE_BUTTON_COUNT];
  bool last_frame_keys[KEY_COUNT];
  bool last_frame_mouse_buttons[MOUSE_BUTTON_COUNT];
  float mouse_x;
  float mouse_y;
  float scroll_x;
  float scroll_y;
  float scroll_x_delta;
  float scroll_y_delta;
  bool closed;
  float pixart_unit;
  uint32_t width;
  uint32_t height;
} Window;

extern Window window;

#endif
