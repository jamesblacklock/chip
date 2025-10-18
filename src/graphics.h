#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <vulkan/vulkan.h>

typedef struct QuadData {
  float w;
  float h;
  float x;
  float y;
  float r;
  float g;
  float b;
  float angle;
} QuadData;

typedef struct TriangleData {
  float ox;
  float oy;
  float x1;
  float y1;
  float x2;
  float y2;
  float x3;
  float y3;
  float r;
  float g;
  float b;
  float angle;
} TriangleData;

void begin_render();
void end_render();
void draw_quad(QuadData data);
void draw_triangle(TriangleData data);
void screen_to_world(float x, float y, float dst[2]);
bool init_vulkan(VkInstance instance, VkSurfaceKHR surface, uint32_t fb_width, uint32_t fb_height);
void cleanup_vulkan();

#endif