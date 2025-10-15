#ifndef MAIN_H
#define MAIN_H

#include <vulkan/vulkan.h>
#include <stdbool.h>

#define byte unsigned char

#define KEY_LSHIFT 0
#define KEY_RSHIFT 1
#define KEY_LCTRL 2
#define KEY_RCTRL 3
#define KEY_LALT 4
#define KEY_RALT 5
#define KEY_LMETA 7
#define KEY_RMETA 8

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

bool init_vulkan(VkInstance instance, VkSurfaceKHR surface, uint32_t fb_width, uint32_t fb_height);
void set_key_state(size_t key, bool state);
void window_closed();
void cleanup();
void set_app_path(const char* path);

bool step(size_t ms);

#endif
