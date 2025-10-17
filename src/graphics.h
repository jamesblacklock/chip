#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <vulkan/vulkan.h>

bool init_vulkan(VkInstance instance, VkSurfaceKHR surface, uint32_t fb_width, uint32_t fb_height);
void cleanup_vulkan();
void render();

typedef struct QuadData {
  float w;
  float h;
  float x;
  float y;
  float r;
  float g;
  float b;
  float a;
} QuadData;

void begin_render();
void end_render();
void draw_quad(QuadData data);
void screen_to_world(float x, float y, float dst[2]);

#endif