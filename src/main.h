#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

#define byte unsigned char

bool init(const char* app_path0, VkInstance instance, VkSurfaceKHR surface, uint32_t width, uint32_t height);
bool tick(size_t ms);
byte* read_file(const char* filename, size_t* size);
void cleanup();

#endif
