#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#define byte unsigned char

#define OBJECT_TYPE_EOF -1
#define OBJECT_TYPE_POLYGON 1

typedef struct SerializableObject {
  int16_t object_type;
} SerializableObject;

bool init(int argc, char** argv, const char* app_path0, VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height);
bool tick(size_t ms);
byte* load_app_resource(const char* filename, size_t* size);
byte* read_file(const char* filename, size_t* size);
void cleanup();

#endif
