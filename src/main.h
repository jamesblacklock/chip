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

typedef struct Program {
  bool (*init)(ExeArgs args);
  bool (*tick)(float ms);
} Program;

bool host_init(int argc, char** argv, const char* app_path0, VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height);
bool host_tick(float ms);
void host_cleanup();
byte* load_app_resource(const char* filename, size_t* size);
byte* read_file(const char* filename, size_t* size);
SerializableObject* read_object(FILE* fp);
void write_eof(FILE* fp);
void write_object(void* object, FILE* fp);

#endif
