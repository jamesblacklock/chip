#include <vulkan/vulkan.h>
#include <vulkan/vulkan_macos.h>

extern VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
extern void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator);
extern VkResult vkCreateMacOSSurfaceMVK(VkInstance instance, const VkMacOSSurfaceCreateInfoMVK* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
extern void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator);

#include "../../main.h"
#include "../../window.h"
