#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include <string.h>

#include "main.h"
#include "graphics.h"
#include "window.h"

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
const size_t MAX_TRIANGLES = 20000;

// MAX_TRIANGLES = 20000
// 10,000 quads
// gfx memory allocation approx. 1.2 MB
// main memory allocation approx. 950 KB

typedef struct VkTrianglePoint {
  vec2 pos;
  uint32_t triangle_id;
} VkTrianglePoint;

typedef VkTrianglePoint VkTriangle[3];

typedef struct VkTriangleAttr {
  mat4 transform;
  vec3 color;
} VkTriangleAttr;

typedef struct UBO {
  mat4 view;
  mat4 proj;
} UBO;

typedef struct RenderContext {
  size_t triangle_index;
  uint32_t swap_index;
} RenderContext;

static uint32_t g_graphics_queue_index;
static VkPhysicalDevice g_physical_device;
static VkSurfaceKHR g_surface;
static uint32_t g_fb_width;
static uint32_t g_fb_height;
static VkDevice g_device;
static VkSwapchainKHR g_swap_chain;
static VkExtent2D g_swap_extent;
static VkSurfaceFormatKHR g_surface_format;
static uint32_t g_swap_image_count;
static VkImage* g_swap_images;
static VkImageView* g_swap_views;
static VkPipelineLayout g_pipeline_layout;
static VkDescriptorSetLayout g_desc_set_layout;
static VkRenderPass g_render_pass;
static VkPipeline g_pipeline;
static VkFramebuffer* g_frame_buffers;
static VkCommandPool g_command_pool;
static VkCommandBuffer g_command_buffer[MAX_FRAMES_IN_FLIGHT];

static VkSemaphore g_sig_ready[MAX_FRAMES_IN_FLIGHT];
static VkSemaphore g_sig_rendered[MAX_FRAMES_IN_FLIGHT];
static VkFence g_fence[MAX_FRAMES_IN_FLIGHT];

static size_t g_frame_index;

static VkDeviceSize g_vbuf_sz = sizeof(VkTriangle) * MAX_TRIANGLES;
static VkBuffer g_vbuf;
static VkDeviceMemory g_vbuf_mem;
static VkBuffer g_vbuf_stg;
static VkDeviceMemory g_vbuf_stg_mem;
static void* g_vbuf_stg_map;

static VkDeviceSize g_sbuf_sz = sizeof(VkTriangleAttr) * MAX_TRIANGLES;
static VkBuffer g_sbuf;
static VkDeviceMemory g_sbuf_mem;
static VkBuffer g_sbuf_stg;
static VkDeviceMemory g_sbuf_stg_mem;
static void* g_sbuf_stg_map;

static VkDeviceSize g_ubuf_sz = sizeof(UBO);
static VkBuffer g_ubuf[MAX_FRAMES_IN_FLIGHT];
static VkDeviceMemory g_ubuf_mem[MAX_FRAMES_IN_FLIGHT];
static void* g_ubuf_map[MAX_FRAMES_IN_FLIGHT];

static VkDescriptorPool g_desc_pool;
static VkDescriptorSet g_desc_set[MAX_FRAMES_IN_FLIGHT];

static TriangleData g_triangle_data[MAX_FRAMES_IN_FLIGHT][MAX_TRIANGLES];

static RenderContext g_ctx[MAX_FRAMES_IN_FLIGHT];

static UBO g_ubo;

static uint32_t clamp(uint32_t n, uint32_t min, uint32_t max) {
  return n > max ? max : n < min ? min : n;
}

static void cleanup_swapchain() {
  for (uint32_t i = 0; i < g_swap_image_count; i++) {
    vkDestroyFramebuffer(g_device, g_frame_buffers[i], NULL);
  }
  for (uint32_t i = 0; i < g_swap_image_count; i++) {
    vkDestroyImageView(g_device, g_swap_views[i], NULL);
  }
  vkDestroySwapchainKHR(g_device, g_swap_chain, NULL);
}

void cleanup_vulkan() {
  vkDeviceWaitIdle(g_device);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(g_device, g_sig_ready[i], NULL);
    vkDestroySemaphore(g_device, g_sig_rendered[i], NULL);
    vkDestroyFence(g_device, g_fence[i], NULL);
  }
  vkDestroyCommandPool(g_device, g_command_pool, NULL);
  vkDestroyPipeline(g_device, g_pipeline, NULL);
  vkDestroyRenderPass(g_device, g_render_pass, NULL);
  cleanup_swapchain();
  vkDestroyBuffer(g_device, g_vbuf, NULL);
  vkFreeMemory(g_device, g_vbuf_mem, NULL);
  vkDestroyBuffer(g_device, g_vbuf_stg, NULL);
  vkFreeMemory(g_device, g_vbuf_stg_mem, NULL);
  vkDestroyBuffer(g_device, g_sbuf, NULL);
  vkFreeMemory(g_device, g_sbuf_mem, NULL);
  vkDestroyBuffer(g_device, g_sbuf_stg, NULL);
  vkFreeMemory(g_device, g_sbuf_stg_mem, NULL);
  vkDestroyPipelineLayout(g_device, g_pipeline_layout, NULL);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(g_device, g_ubuf[i], NULL);
    vkFreeMemory(g_device, g_ubuf_mem[i], NULL);
  }
  vkDestroyDescriptorSetLayout(g_device, g_desc_set_layout, NULL);
  vkDestroyDevice(g_device, NULL);

  printf("cleanup complete\n");
}

static VkShaderModule load_shader(const char* filename) {
  size_t size;
  byte* bytes = read_file(filename, &size);
  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = (uint32_t*) bytes,
  };
  VkShaderModule module;
  if (vkCreateShaderModule(g_device, &create_info, NULL, &module) != VK_SUCCESS) {
    return NULL;
  }
  return module;
}

static VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR* caps, uint32_t fb_width, uint32_t fb_height) {
  if (caps->currentExtent.width != UINT32_MAX) {
    return caps->currentExtent;
  } else {
    // TODO: update fb_width/height!
    VkExtent2D extent = {
      .width = fb_width,
      .height = fb_height,
    };

    extent.width = clamp(extent.width, caps->minImageExtent.width, caps->maxImageExtent.width);
    extent.height = clamp(extent.height, caps->minImageExtent.height, caps->maxImageExtent.height);

    return extent;
  }
}

static bool create_swapchain() {
  if (g_swap_chain != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(g_device);
    cleanup_swapchain();
    g_swap_chain = VK_NULL_HANDLE;
  }

  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physical_device, g_surface, &caps);
  g_swap_extent = choose_swap_extent(&caps, g_fb_width, g_fb_height);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical_device, g_surface, &format_count, NULL);
  if (format_count == 0) {
    fprintf(stderr, "format_count == 0\n");
    return false;
  }
  VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical_device, g_surface, &format_count, formats);
  bool found_surface_format = false;
  for (uint32_t i = 0; i < format_count; i++) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      found_surface_format = true;
      g_surface_format = formats[i];
      break;
    }
  }
  if (!found_surface_format) {
    g_surface_format = formats[0];
  }

  uint32_t present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical_device, g_surface, &present_modes_count, NULL);
  if (present_modes_count == 0) {
    fprintf(stderr, "present_modes_count == 0\n");
    return false;
  }
  VkPresentModeKHR* present_modes = malloc(sizeof(VkPresentModeKHR) * present_modes_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical_device, g_surface, &present_modes_count, present_modes);
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32_t i = 0; i < present_modes_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = present_modes[i];
      break;
    }
  }

  uint32_t image_count = clamp(caps.minImageCount + 1, caps.minImageCount, caps.maxImageCount);

  VkSwapchainCreateInfoKHR swapchain_create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = g_surface,
    .minImageCount = image_count,
    .imageFormat = g_surface_format.format,
    .imageColorSpace = g_surface_format.colorSpace,
    .imageExtent = g_swap_extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform = caps.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = present_mode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };
  if (vkCreateSwapchainKHR(g_device, &swapchain_create_info, NULL, &g_swap_chain) != VK_SUCCESS) {
    fprintf(stderr, "failed to create swapchain\n");
    return false;
  }

  vkGetSwapchainImagesKHR(g_device, g_swap_chain, &g_swap_image_count, NULL);
  g_swap_images = malloc(sizeof(VkImage) * g_swap_image_count);
  vkGetSwapchainImagesKHR(g_device, g_swap_chain, &g_swap_image_count, g_swap_images);

  g_swap_views = malloc(sizeof(VkImageView) * g_swap_image_count);
  for (uint32_t i = 0; i < g_swap_image_count; i++) {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = g_swap_images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = g_surface_format.format,
      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
      },
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };
    if (vkCreateImageView(g_device, &create_info, NULL, &g_swap_views[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create swap image views\n");
      return false;
    }
  }

  g_frame_buffers = malloc(sizeof(VkFramebuffer) * g_swap_image_count);
  for (uint32_t i = 0; i < g_swap_image_count; i++) {
    VkFramebufferCreateInfo framebufferInfo = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = g_render_pass,
      .attachmentCount = 1,
      .pAttachments = &g_swap_views[i],
      .width = g_swap_extent.width,
      .height = g_swap_extent.height,
      .layers = 1,
    };

    if (vkCreateFramebuffer(g_device, &framebufferInfo, NULL, &g_frame_buffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create frame buffers\n");
      return false;
    }
  }

  glm_mat4_identity(g_ubo.view);
  float aspect = g_swap_extent.width / (float) g_swap_extent.height;
  glm_ortho(-aspect, aspect, -1, 1, -1, 1, g_ubo.proj);

  printf("Vulkan swapchain setup complete\n");

  return true;
}

int32_t find_memory_type(VkPhysicalDeviceMemoryProperties mem_props, uint32_t filter, VkMemoryPropertyFlags properties) {
  for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
    if (filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & properties)) {
        return i;
    }
  }
  return -1;
}

static bool create_buffer(VkBuffer* buf, VkDeviceMemory* mem, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props) {
  VkBufferCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  if (vkCreateBuffer(g_device, &create_info, NULL, buf) != VK_SUCCESS) {
    fprintf(stderr, "failed to create buffer\n");
    return false;
  }

  VkMemoryRequirements mem_req;
  vkGetBufferMemoryRequirements(g_device, *buf, &mem_req);
  VkPhysicalDeviceMemoryProperties dev_mem_props;
  vkGetPhysicalDeviceMemoryProperties(g_physical_device, &dev_mem_props);

  int32_t mem_type_index = find_memory_type(dev_mem_props, mem_req.memoryTypeBits, mem_props);
  if (mem_type_index < 0) {
    fprintf(stderr, "no available memory type for buffer\n");
    return false;
  }

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_req.size,
    .memoryTypeIndex = mem_type_index,
  };
  if (vkAllocateMemory(g_device, &alloc_info, NULL, mem) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate buffer memory\n");
    return false;
  }
  vkBindBufferMemory(g_device, *buf, *mem, 0);

  return true;
}

static VkQueue get_graphics_queue() {
  VkQueue queue;
  vkGetDeviceQueue(g_device, g_graphics_queue_index, 0, &queue);
  return queue;
}

static void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize sz) {
  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandPool = g_command_pool,
    .commandBufferCount = 1,
  };

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(g_device, &alloc_info, &cmd);

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(cmd, &begin_info);
  vkCmdCopyBuffer(cmd, src, dst, 1, &(VkBufferCopy) { .size = sz });
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &cmd,
  };
  VkQueue queue = get_graphics_queue();
  vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);

  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(g_device, g_command_pool, 1, &cmd);
}

bool init_vulkan(VkInstance instance, VkSurfaceKHR surface, uint32_t fb_width, uint32_t fb_height) {
  g_surface = surface;
  g_fb_width = fb_width;
  g_fb_height = fb_height;

  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance, &device_count, NULL);
  if (device_count < 1) {
    fprintf(stderr, "no device available\n");
    return false;
  }
  vkEnumeratePhysicalDevices(instance, &device_count, &g_physical_device);

  uint32_t queue_family_count;
  vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_family_count, NULL);
  if (queue_family_count < 1) {
    fprintf(stderr, "queue_family_count < 1\n");
    return false;
  }
  VkQueueFamilyProperties* queue_familes = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_family_count, queue_familes);
  bool found_graphics_queue_index = false;
  for (uint32_t i = 0; i < queue_family_count; i++) {
    if (queue_familes[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      VkBool32 can_present;
      vkGetPhysicalDeviceSurfaceSupportKHR(g_physical_device, i, surface, &can_present);
      if (can_present) {
        found_graphics_queue_index = true;
        g_graphics_queue_index = i;
        break;
      }
    }
  }
  if (!found_graphics_queue_index) {
    fprintf(stderr, "found_graphics_queue_index == false\n");
    return false;
  }

  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(g_physical_device, &features);
  
  float priority = 1.0;
  VkDeviceQueueCreateInfo queueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = g_graphics_queue_index,
    .queueCount = 1,
    .pQueuePriorities = &priority,
  };
  const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  VkDeviceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queueCreateInfo,
    .pEnabledFeatures = &features,
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = extensions,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = NULL,
  };
  if (vkCreateDevice(g_physical_device, &create_info, NULL, &g_device) != VK_SUCCESS) {
    fprintf(stderr, "failed to create logical device\n");
    return false;
  }

  if (!create_swapchain()) {
    return false;
  }

  VkShaderModule vert = load_shader("vert.spv");
  if (!vert) {
    fprintf(stderr, "failed to load shader module \"vert.spv\"\n");
    return false;
  }
  VkShaderModule frag = load_shader("frag.spv");
  if (!frag) {
    fprintf(stderr, "failed to load shader module \"frag.spv\"\n");
    return false;
  }

  VkPipelineShaderStageCreateInfo shader_stages[] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vert,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = frag,
      .pName = "main",
    }
  };

  VkDynamicState dynamic_state_info[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 2,
    .pDynamicStates = dynamic_state_info,
  };

  VkVertexInputBindingDescription input_desc = {
    .binding = 0,
    .stride = sizeof(VkTrianglePoint),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputAttributeDescription attr_desc[] = {
    {
      .binding = 0,
      .location = 0,
      .format = VK_FORMAT_R32G32_SFLOAT,
      .offset = offsetof(VkTrianglePoint, pos),
    },
    {
      .binding = 0,
      .location = 1,
      .format = VK_FORMAT_R32_UINT,
      .offset = offsetof(VkTrianglePoint, triangle_id),
    },
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &input_desc,
    .vertexAttributeDescriptionCount = 2,
    .pVertexAttributeDescriptions = attr_desc
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float) g_swap_extent.width,
    .height = (float) g_swap_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };

  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = g_swap_extent,
  };

  VkPipelineViewportStateCreateInfo viewport_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
  };

  VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f,
    .pSampleMask = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState blend_attachment = {
    .blendEnable = VK_TRUE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
  };

  VkPipelineColorBlendStateCreateInfo blend = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &blend_attachment,
    .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
  };

  VkDescriptorSetLayoutBinding ubo_layout_binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };
  VkDescriptorSetLayoutBinding attr_layout_binding = {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };
  VkDescriptorSetLayoutBinding bindings[2] = {ubo_layout_binding, attr_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 2,
    .pBindings = bindings,
  };
  if (vkCreateDescriptorSetLayout(g_device, &layout_info, NULL, &g_desc_set_layout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor set layout\n");
    return false;
  }

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &g_desc_set_layout,
    .pushConstantRangeCount = 0,
  };

  if (vkCreatePipelineLayout(g_device, &pipeline_layout_info, NULL, &g_pipeline_layout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create pipeline layout\n");
    return false;
  }

  VkAttachmentDescription color_attachment = {
    .format = g_surface_format.format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference color_attachment_ref = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDependency dep = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_ref,
  };

  VkRenderPassCreateInfo render_pass_create_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color_attachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dep,
  };
  if (vkCreateRenderPass(g_device, &render_pass_create_info, NULL, &g_render_pass) != VK_SUCCESS) {
    fprintf(stderr, "failed to create render pass\n");
    return false;
  }

  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shader_stages,
    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pColorBlendState = &blend,
    .pDynamicState = &dynamic_state,
    .layout = g_pipeline_layout,
    .renderPass = g_render_pass,
    .subpass = 0,
  };

  if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &g_pipeline) != VK_SUCCESS) {
    fprintf(stderr, "failed to create graphics pipeline\n");
    return false;
  }

  printf("Created Vulkan graphics pipeline\n");

  vkDestroyShaderModule(g_device, vert, NULL);
  vkDestroyShaderModule(g_device, frag, NULL);

  VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = g_graphics_queue_index,
  };
  if (vkCreateCommandPool(g_device, &pool_info, NULL, &g_command_pool) != VK_SUCCESS) {
    fprintf(stderr, "failed to command pool\n");
    return false;
  }

  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = g_command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };
  if (vkAllocateCommandBuffers(g_device, &alloc_info, g_command_buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to command buffers\n");
    return false;
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(g_device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &g_sig_ready[i]) != VK_SUCCESS
      || vkCreateSemaphore(g_device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &g_sig_rendered[i]) != VK_SUCCESS
      || vkCreateFence(g_device, &(VkFenceCreateInfo) { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT }, NULL, &g_fence[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create sync primitives\n");
    }
  }

  // vertex buffer setup
  VkBufferUsageFlags vbuf_stg_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags vbuf_stg_mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (!create_buffer(&g_vbuf_stg, &g_vbuf_stg_mem, g_vbuf_sz, vbuf_stg_usage, vbuf_stg_mem_props)) {
    return false;
  }
  vkMapMemory(g_device, g_vbuf_stg_mem, 0, g_vbuf_sz, 0, &g_vbuf_stg_map);

  VkBufferUsageFlags vbuf_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  VkMemoryPropertyFlags vbuf_mem_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!create_buffer(&g_vbuf, &g_vbuf_mem, g_vbuf_sz, vbuf_usage, vbuf_mem_props)) {
    return false;
  }

  // storage buffer setup
  VkBufferUsageFlags sbuf_stg_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags sbuf_stg_mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (!create_buffer(&g_sbuf_stg, &g_sbuf_stg_mem, g_sbuf_sz, sbuf_stg_usage, sbuf_stg_mem_props)) {
    return false;
  }
  vkMapMemory(g_device, g_sbuf_stg_mem, 0, g_sbuf_sz, 0, &g_sbuf_stg_map);

  VkBufferUsageFlags sbuf_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  VkMemoryPropertyFlags sbuf_mem_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!create_buffer(&g_sbuf, &g_sbuf_mem, g_sbuf_sz, sbuf_usage, sbuf_mem_props)) {
    return false;
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    // uniform buffer setup
    VkBufferUsageFlags ubuf_stg_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryPropertyFlags ubuf_stg_mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (!create_buffer(&g_ubuf[i], &g_ubuf_mem[i], g_ubuf_sz, ubuf_stg_usage, ubuf_stg_mem_props)) {
      return false;
    }
    vkMapMemory(g_device, g_ubuf_mem[i], 0, g_ubuf_sz, 0, &g_ubuf_map[i]);
  }

  VkDescriptorPoolCreateInfo desc_pool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = 1,
    .pPoolSizes = &(VkDescriptorPoolSize) {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    },
    .maxSets = MAX_FRAMES_IN_FLIGHT,
  };
  if (vkCreateDescriptorPool(g_device, &desc_pool_info, NULL, &g_desc_pool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor pool\n");
    return false;
  }

  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {g_desc_set_layout, g_desc_set_layout};
  VkDescriptorSetAllocateInfo desc_set_alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = g_desc_pool,
    .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
    .pSetLayouts = layouts,
  };
  if (vkAllocateDescriptorSets(g_device, &desc_set_alloc_info, g_desc_set) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate descriptor sets\n");
  }
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo ubuf_info = {
      .buffer = g_ubuf[i],
      .offset = 0,
      .range = g_ubuf_sz,
    };
    VkDescriptorBufferInfo sbuf_info = {
      .buffer = g_sbuf,
      .offset = 0,
      .range = g_sbuf_sz,
    };
    VkWriteDescriptorSet desc_write[2] = {
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = g_desc_set[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &ubuf_info,
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = g_desc_set[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &sbuf_info,
      },
    };
    vkUpdateDescriptorSets(g_device, 2, desc_write, 0, NULL);
  }

  return true;
}

void screen_to_world(float x, float y, vec2 dst) {
  mat4 mat;
  glm_mat4_mul(g_ubo.proj, g_ubo.view, mat);
  glm_mat4_inv(mat, mat);
  vec4 vec = {x/(g_swap_extent.width/2), y/(g_swap_extent.height/2), 1, 1};
  glm_mat4_mulv(mat, vec, vec);
  vec[3] = 1 / vec[3];
  vec[0] *= vec[3];
  vec[1] *= vec[3];
  dst[0] = vec[0];
  dst[1] = vec[1];
}
float screen_x_to_world(float x) {
  vec2 res;
  screen_to_world(x, 0, res);
  return res[0];
}
float screen_y_to_world(float y) {
  vec2 res;
  screen_to_world(0, y, res);
  return res[1];
}

void begin_render() {
  VkFence fence = g_fence[g_frame_index];
  VkSemaphore sig_ready = g_sig_ready[g_frame_index];

  vkWaitForFences(g_device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(g_device, 1, &fence);

  uint32_t swap_index = 0;
  VkResult acquire_image_res = vkAcquireNextImageKHR(g_device, g_swap_chain, UINT64_MAX, sig_ready, VK_NULL_HANDLE, &swap_index);
  if (acquire_image_res == VK_ERROR_OUT_OF_DATE_KHR || acquire_image_res == VK_SUBOPTIMAL_KHR) {
    create_swapchain();
  } else if (acquire_image_res != VK_SUCCESS) {
    fprintf(stderr, "failed to acquire swapchain image\n");
    return;
  }

  g_ctx[g_frame_index].triangle_index = 0;
  g_ctx[g_frame_index].swap_index = swap_index;
}

void draw_quad(QuadData data) {
  RenderContext* ctx = &g_ctx[g_frame_index];
  if (ctx->triangle_index + 1 >= MAX_TRIANGLES) {
    fprintf(stderr, "ran out of vertex buffer space\n");
    return;
  }

  float x1 = -data.w/2;
  float x2 = +data.w/2;
  float y1 = -data.h/2;
  float y2 = +data.h/2;

  g_triangle_data[g_frame_index][ctx->triangle_index++] = (TriangleData){
    .ox = data.x, .oy = data.y, .angle = data.angle,
    .x1 = x1, .y1 = y1,
    .x2 = x1, .y2 = y2,
    .x3 = x2, .y3 = y2,
    .r = data.r, .g = data.g, .b = data.b,
  };
  g_triangle_data[g_frame_index][ctx->triangle_index++] = (TriangleData){
    .ox = data.x, .oy = data.y, .angle = data.angle,
    .x1 = x1, .y1 = y1,
    .x2 = x2, .y2 = y2,
    .x3 = x2, .y3 = y1,
    .r = data.r, .g = data.g, .b = data.b,
  };
}
static float dist(float x, float y) {
  return sqrt(x*x + y*y);
}
void draw_line(LineData data) {
  float dx = data.x2 - data.x1;
  float dy = data.y2 - data.y1;
  draw_quad((QuadData){
    .x = dx/2 + data.x1,
    .y = dy/2 + data.y1,
    .w = dist(dx, dy),
    .h = data.w,
    .angle = atan(dy/dx),
    .r = data.r,
    .g = data.g,
    .b = data.b,
  });
}
void draw_triangle(TriangleData data) {
  RenderContext* ctx = &g_ctx[g_frame_index];
  if (ctx->triangle_index >= MAX_TRIANGLES) {
    fprintf(stderr, "ran out of vertex buffer space\n");
    return;
  }
  g_triangle_data[g_frame_index][ctx->triangle_index++] = data;
}

void end_render() {
  RenderContext* ctx = &g_ctx[g_frame_index];
  memcpy(g_ubuf_map[g_frame_index], &g_ubo, sizeof(UBO));

  VkTriangle* vbuf_stg_map = g_vbuf_stg_map;
  VkTriangleAttr* sbuf_stg_map = g_sbuf_stg_map;
  TriangleData* triangles = g_triangle_data[g_frame_index];
  for (size_t i=0; i < ctx->triangle_index; i++) {
    TriangleData* data = &triangles[i];
    VkTriangle vector_data = {
      {{data->x1, data->y1}, .triangle_id = i},
      {{data->x2, data->y2}, .triangle_id = i},
      {{data->x3, data->y3}, .triangle_id = i},
    };
    memcpy(vbuf_stg_map, &vector_data, sizeof(VkTriangle));
    vbuf_stg_map++;

    VkTriangleAttr attr_data = { .color = {data->r, data->g, data->b} };
    glm_mat4_identity(attr_data.transform);
    vec3 point = {data->ox, data->oy, 0};
    glm_translate(attr_data.transform, point);
    glm_rotate(attr_data.transform, data->angle, (vec3){0,0,1});
    memcpy(sbuf_stg_map, &attr_data, sizeof(VkTriangleAttr));
    sbuf_stg_map++;
  }
  copy_buffer(g_vbuf_stg, g_vbuf, sizeof(VkTriangle) * ctx->triangle_index);
  copy_buffer(g_sbuf_stg, g_sbuf, sizeof(VkTriangleAttr) * ctx->triangle_index);

  VkCommandBuffer command_buffer = g_command_buffer[g_frame_index];
  vkResetCommandBuffer(command_buffer, 0);
  if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }) != VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer\n");
  }
  VkClearValue clear = { .color = { .float32 = {0.0f, 0.0f, 0.0f, 1.0f} } };
  VkRenderPassBeginInfo pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = g_render_pass,
    .framebuffer = g_frame_buffers[ctx->swap_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = g_swap_extent,
    },
    .clearValueCount = 1,
    .pClearValues = &clear,
  };
  vkCmdBeginRenderPass(command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);

  VkDeviceSize vbuf_offsets = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &g_vbuf, &vbuf_offsets);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline_layout, 0, 1, &g_desc_set[g_frame_index], 0, NULL);

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float) g_swap_extent.width,
    .height = (float) g_swap_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = g_swap_extent,
  };
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  vkCmdDraw(command_buffer, 3 * ctx->triangle_index, 1, 0, 0);
  vkCmdEndRenderPass(command_buffer);
  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to record command buffer\n");
  }

  VkQueue queue = get_graphics_queue();

  VkSemaphore sig_ready = g_sig_ready[g_frame_index];
  VkSemaphore sig_rendered = g_sig_rendered[g_frame_index];
  VkFence fence = g_fence[g_frame_index];
  VkSubmitInfo submit = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &sig_ready,
    .pWaitDstStageMask = &(VkPipelineStageFlags) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    .commandBufferCount = 1,
    .pCommandBuffers = &command_buffer,
    .pSignalSemaphores = &sig_rendered,
    .signalSemaphoreCount = 1,
  };
  if (vkQueueSubmit(queue, 1, &submit, fence) != VK_SUCCESS) {
    fprintf(stderr, "failed to submit draw command buffer\n");
  }

  VkPresentInfoKHR present = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &sig_rendered,
    .swapchainCount = 1,
    .pSwapchains = &g_swap_chain,
    .pImageIndices = &ctx->swap_index,
  };
  vkQueuePresentKHR(queue, &present);

  g_frame_index = (g_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}
