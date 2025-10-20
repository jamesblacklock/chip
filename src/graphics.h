#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <vulkan/vulkan.h>

typedef struct QuadData {
  float w;
  float h;
  float x;
  float y;
  float z;
  float r;
  float g;
  float b;
  float angle;
} QuadData;

typedef struct LineData {
  float x1;
  float y1;
  float x2;
  float y2;
  float z;
  float w;
  float r;
  float g;
  float b;
} LineData;

typedef struct TriangleData {
  float ox;
  float oy;
  float x1;
  float y1;
  float x2;
  float y2;
  float x3;
  float y3;
  float z;
  float r;
  float g;
  float b;
  float angle;
  bool reuse_attrs;
} TriangleData;

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C 
#endif

EXTERN_C void begin_render();
EXTERN_C void end_render();
EXTERN_C void set_view_coords(float x, float y, float z);
EXTERN_C void draw_quad(QuadData data);
EXTERN_C void draw_triangle(TriangleData data);
EXTERN_C void draw_line(LineData data);
EXTERN_C float screen_to_z0(float n);
EXTERN_C bool init_vulkan(VkInstance instance, VkSurfaceKHR surface, uint32_t fb_width, uint32_t fb_height);
void surface_dimensions_changed(uint32_t fb_width, uint32_t fb_height);
EXTERN_C void cleanup_vulkan();

#endif