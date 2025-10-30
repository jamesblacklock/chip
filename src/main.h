#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <stdio.h>

#define byte unsigned char

#define OBJECT_TYPE_EOF -1
#define OBJECT_TYPE_POLYGON 1

typedef struct SerializableObject {
  int16_t object_type;
} SerializableObject;

typedef struct ExeArgs {
  bool valid;
  struct {
    const char* filename;
  } map_editor;
  struct {
    const char* mapfile;
  } play;
} ExeArgs;

typedef struct Vec2 {
  float x;
  float y;
} Vec2;

typedef struct Color {
  float r;
  float g;
  float b;
} Color;

typedef struct Program {
  bool (*init)(ExeArgs args);
  bool (*tick)(float ms);
} Program;

bool host_init(int argc, char** argv, const char* app_path0, VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height);
bool host_tick(float ms);
void host_cleanup();
void mouse_viewport();
byte* load_app_resource(const char* filename, size_t* size);
byte* read_file(const char* filename, size_t* size);
SerializableObject* read_object(FILE* fp);
void write_eof(FILE* fp);
void write_object(void* object, FILE* fp);

static inline float cross_product(Vec2 a, Vec2 b) {
  return (a.x*b.y) - (a.y*b.x);
}

static inline float fclamp(float n, float min, float max) {
  return n > max ? max : n < min ? min : n;
}

static inline bool z(float n) {
  return n < 0.0001 && n > -0.0001;
}

static inline bool between(float n, float min, float max) {
  return z(n - max) ? true : z(n - min) ? true : n > max ? false : n < min ? false : true;
}

#endif
