#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <vulkan/vulkan.h>

uint32_t clamp(uint32_t n, uint32_t min, uint32_t max) {
  return n > max ? max : n < min ? min : n;
}

char* app_path;

bool keys[500] = {};
bool quit;
bool thing;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct {
  uint32_t graphics_queue_index;
  VkPhysicalDevice physical_device;
  VkSurfaceKHR surface;
  uint32_t fb_width;
  uint32_t fb_height;
  VkDevice device;
  VkSwapchainKHR swap_chain;
  VkExtent2D swap_extent;
  VkSurfaceFormatKHR surface_format;
  uint32_t swap_image_count;
  VkImage* swap_images;
  VkImageView* swap_views;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  VkPipeline pipeline;
  VkFramebuffer* frame_buffers;
  VkCommandPool command_pool;
  VkCommandBuffer command_buffer[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore sig_ready[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore sig_rendered[MAX_FRAMES_IN_FLIGHT];
  VkFence fence[MAX_FRAMES_IN_FLIGHT];
  size_t frame_index;
} Vulkan;

void set_key_state(size_t key, bool state) {
  keys[key] = state;
}

void set_app_path(const char* path) {
  size_t len = strlen(path);
  app_path = malloc(len + 1);
  strncpy(app_path, path, len);
  app_path[len] = 0;
}

void cleanup_swapchain() {
  for (uint32_t i = 0; i < Vulkan.swap_image_count; i++) {
    vkDestroyFramebuffer(Vulkan.device, Vulkan.frame_buffers[i], NULL);
  }
  for (uint32_t i = 0; i < Vulkan.swap_image_count; i++) {
    vkDestroyImageView(Vulkan.device, Vulkan.swap_views[i], NULL);
  }
  vkDestroySwapchainKHR(Vulkan.device, Vulkan.swap_chain, NULL);
}

void cleanup() {
  vkDeviceWaitIdle(Vulkan.device);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(Vulkan.device, Vulkan.sig_ready[i], NULL);
    vkDestroySemaphore(Vulkan.device, Vulkan.sig_rendered[i], NULL);
    vkDestroyFence(Vulkan.device, Vulkan.fence[i], NULL);
  }
  vkDestroyCommandPool(Vulkan.device, Vulkan.command_pool, NULL);
  vkDestroyPipeline(Vulkan.device, Vulkan.pipeline, NULL);
  vkDestroyRenderPass(Vulkan.device, Vulkan.render_pass, NULL);
  cleanup_swapchain();
  vkDestroyPipelineLayout(Vulkan.device, Vulkan.pipeline_layout, NULL);
  vkDestroyRenderPass(Vulkan.device, Vulkan.render_pass, NULL);
  vkDestroyDevice(Vulkan.device, NULL);

  printf("cleanup complete\n");
}

byte* read_file(const char* filename, size_t* size) {
  char* absolute_path = malloc(strlen(filename) + strlen(app_path) + 1);
  absolute_path[0] = 0;
  strcat(absolute_path, app_path);
  strcat(absolute_path, filename);
  // printf("%s\n", app_path);

  struct stat info;
  int res = stat(absolute_path, &info);
  if (res != 0) {
    fprintf(stderr, "failed to stat file: %s\n", absolute_path);
    exit(1);
  }
  byte* buf = malloc(info.st_size);
  FILE* file = fopen(absolute_path, "rb");
  if (file == NULL) {
    fprintf(stderr, "failed to open file: %s\n", absolute_path);
    exit(1);
  }
  size_t bytes_read = fread(buf, 1, info.st_size, file);
  if (bytes_read != info.st_size) {
    fprintf(stderr, "failed to read file: %s\n", absolute_path);
    exit(1);
  }
  *size = bytes_read;
  return buf;
}

VkShaderModule load_shader(const char* filename) {
  size_t size;
  byte* bytes = read_file(filename, &size);
  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = (uint32_t*) bytes,
  };
  VkShaderModule module;
  if (vkCreateShaderModule(Vulkan.device, &create_info, NULL, &module) != VK_SUCCESS) {
    return NULL;
  }
  return module;
}

VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR* caps, uint32_t fb_width, uint32_t fb_height) {
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

bool create_swapchain() {
  if (Vulkan.swap_chain != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(Vulkan.device);
    cleanup_swapchain();
    Vulkan.swap_chain = VK_NULL_HANDLE;
  }

  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan.physical_device, Vulkan.surface, &caps);
  Vulkan.swap_extent = choose_swap_extent(&caps, Vulkan.fb_width, Vulkan.fb_height);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan.physical_device, Vulkan.surface, &format_count, NULL);
  if (format_count == 0) {
    fprintf(stderr, "format_count == 0\n");
    return false;
  }
  VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan.physical_device, Vulkan.surface, &format_count, formats);
  bool found_surface_format = false;
  for (uint32_t i = 0; i < format_count; i++) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      found_surface_format = true;
      Vulkan.surface_format = formats[i];
      break;
    }
  }
  if (!found_surface_format) {
    Vulkan.surface_format = formats[0];
  }

  uint32_t present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.physical_device, Vulkan.surface, &present_modes_count, NULL);
  if (present_modes_count == 0) {
    fprintf(stderr, "present_modes_count == 0\n");
    return false;
  }
  VkPresentModeKHR* present_modes = malloc(sizeof(VkPresentModeKHR) * present_modes_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.physical_device, Vulkan.surface, &present_modes_count, present_modes);
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
    .surface = Vulkan.surface,
    .minImageCount = image_count,
    .imageFormat = Vulkan.surface_format.format,
    .imageColorSpace = Vulkan.surface_format.colorSpace,
    .imageExtent = Vulkan.swap_extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform = caps.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = present_mode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };
  if (vkCreateSwapchainKHR(Vulkan.device, &swapchain_create_info, NULL, &Vulkan.swap_chain) != VK_SUCCESS) {
    fprintf(stderr, "failed to create swapchain\n");
    return false;
  }

  vkGetSwapchainImagesKHR(Vulkan.device, Vulkan.swap_chain, &Vulkan.swap_image_count, NULL);
  Vulkan.swap_images = malloc(sizeof(VkImage) * Vulkan.swap_image_count);
  vkGetSwapchainImagesKHR(Vulkan.device, Vulkan.swap_chain, &Vulkan.swap_image_count, Vulkan.swap_images);

  Vulkan.swap_views = malloc(sizeof(VkImageView) * Vulkan.swap_image_count);
  for (uint32_t i = 0; i < Vulkan.swap_image_count; i++) {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = Vulkan.swap_images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = Vulkan.surface_format.format,
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
    if (vkCreateImageView(Vulkan.device, &create_info, NULL, &Vulkan.swap_views[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create swap image views\n");
      return false;
    }
  }

  Vulkan.frame_buffers = malloc(sizeof(VkFramebuffer) * Vulkan.swap_image_count);
  for (uint32_t i = 0; i < Vulkan.swap_image_count; i++) {
    VkFramebufferCreateInfo framebufferInfo = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = Vulkan.render_pass,
      .attachmentCount = 1,
      .pAttachments = &Vulkan.swap_views[i],
      .width = Vulkan.swap_extent.width,
      .height = Vulkan.swap_extent.height,
      .layers = 1,
    };

    if (vkCreateFramebuffer(Vulkan.device, &framebufferInfo, NULL, &Vulkan.frame_buffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create frame buffers\n");
      return false;
    }
  }

  printf("Vulkan swapchain setup complete\n");

  return true;
}

bool init_vulkan(VkInstance instance, VkSurfaceKHR surface, uint32_t fb_width, uint32_t fb_height) {
  Vulkan.surface = surface;
  Vulkan.fb_width = fb_width;
  Vulkan.fb_height = fb_height;

  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance, &device_count, &Vulkan.physical_device);
  if (device_count < 1) {
    fprintf(stderr, "no device available\n");
    return false;
  }

  uint32_t queue_family_count;
  vkGetPhysicalDeviceQueueFamilyProperties(Vulkan.physical_device, &queue_family_count, NULL);
  if (queue_family_count < 1) {
    fprintf(stderr, "queue_family_count < 1\n");
    return false;
  }
  VkQueueFamilyProperties* queue_familes = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(Vulkan.physical_device, &queue_family_count, queue_familes);
  bool found_graphics_queue_index = false;
  for (uint32_t i = 0; i < queue_family_count; i++) {
    if (queue_familes[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      VkBool32 can_present;
      vkGetPhysicalDeviceSurfaceSupportKHR(Vulkan.physical_device, i, surface, &can_present);
      if (can_present) {
        found_graphics_queue_index = true;
        Vulkan.graphics_queue_index = i;
        break;
      }
    }
  }
  if (!found_graphics_queue_index) {
    fprintf(stderr, "found_graphics_queue_index == false\n");
    return false;
  }

  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(Vulkan.physical_device, &features);
  
  float priority = 1.0;
  VkDeviceQueueCreateInfo queueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = Vulkan.graphics_queue_index,
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
  if (vkCreateDevice(Vulkan.physical_device, &create_info, NULL, &Vulkan.device) != VK_SUCCESS) {
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

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 0,
    .vertexAttributeDescriptionCount = 0,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float) Vulkan.swap_extent.width,
    .height = (float) Vulkan.swap_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };

  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = Vulkan.swap_extent,
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
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
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

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0,
    .pSetLayouts = NULL,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL,
  };

  if (vkCreatePipelineLayout(Vulkan.device, &pipeline_layout_info, NULL, &Vulkan.pipeline_layout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create pipeline layout\n");
    return false;
  }

  VkAttachmentDescription color_attachment = {
    .format = Vulkan.surface_format.format,
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

  if (vkCreateRenderPass(Vulkan.device, &render_pass_create_info, NULL, &Vulkan.render_pass) != VK_SUCCESS) {
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
    .layout = Vulkan.pipeline_layout,
    .renderPass = Vulkan.render_pass,
    .subpass = 0,
  };

  if (vkCreateGraphicsPipelines(Vulkan.device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &Vulkan.pipeline) != VK_SUCCESS) {
    fprintf(stderr, "failed to create graphics pipeline\n");
    return false;
  }

  printf("Created Vulkan graphics pipeline\n");

  vkDestroyShaderModule(Vulkan.device, vert, NULL);
  vkDestroyShaderModule(Vulkan.device, frag, NULL);

  VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = Vulkan.graphics_queue_index,
  };
  if (vkCreateCommandPool(Vulkan.device, &pool_info, NULL, &Vulkan.command_pool) != VK_SUCCESS) {
    fprintf(stderr, "failed to command pool\n");
    return false;
  }

  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = Vulkan.command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };
  if (vkAllocateCommandBuffers(Vulkan.device, &alloc_info, Vulkan.command_buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to command buffers\n");
    return false;
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(Vulkan.device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &Vulkan.sig_ready[i]) != VK_SUCCESS
      || vkCreateSemaphore(Vulkan.device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &Vulkan.sig_rendered[i]) != VK_SUCCESS
      || vkCreateFence(Vulkan.device, &(VkFenceCreateInfo) { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT }, NULL, &Vulkan.fence[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create sync primitives\n");
    }
  }

  return true;
}

VkQueue get_graphics_queue() {
  VkQueue queue;
  vkGetDeviceQueue(Vulkan.device, Vulkan.graphics_queue_index, 0, &queue);
  return queue;
}

void render() {
  VkFence fence = Vulkan.fence[Vulkan.frame_index];
  VkSemaphore sig_ready = Vulkan.sig_ready[Vulkan.frame_index];
  VkSemaphore sig_rendered = Vulkan.sig_rendered[Vulkan.frame_index];
  VkCommandBuffer command_buffer = Vulkan.command_buffer[Vulkan.frame_index];

  vkWaitForFences(Vulkan.device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(Vulkan.device, 1, &fence);

  uint32_t swap_index = 0;
  VkResult acquire_image_res = vkAcquireNextImageKHR(Vulkan.device, Vulkan.swap_chain, UINT64_MAX, sig_ready, VK_NULL_HANDLE, &swap_index);
  if (acquire_image_res == VK_ERROR_OUT_OF_DATE_KHR) {
    create_swapchain();
    return;
  } else if (acquire_image_res != VK_SUCCESS) {
    fprintf(stderr, "failed to acquire swapchain image\n");
    return;
  }

  vkResetCommandBuffer(command_buffer, 0);
  if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }) != VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer\n");
  }
  VkClearValue clear = { .color = { .float32 = {0.0f, 0.0f, 0.0f, 1.0f} } };
  VkRenderPassBeginInfo pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = Vulkan.render_pass,
    .framebuffer = Vulkan.frame_buffers[swap_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = Vulkan.swap_extent,
    },
    .clearValueCount = 1,
    .pClearValues = &clear,
  };
  vkCmdBeginRenderPass(command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Vulkan.pipeline);

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float) Vulkan.swap_extent.width,
    .height = (float) Vulkan.swap_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = Vulkan.swap_extent,
  };
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  vkCmdDraw(command_buffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(command_buffer);
  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to record command buffer\n");
  }

  VkQueue queue = get_graphics_queue();

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
    .pSwapchains = &Vulkan.swap_chain,
    .pImageIndices = &swap_index,
  };
  vkQueuePresentKHR(queue, &present);

  Vulkan.frame_index = (Vulkan.frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool step(size_t ms) {
  if (quit) {
    return false;
  }

  render();

  if (keys[KEY_LALT] && !thing) {
    printf("%ld yes\n", ms);
    thing = true;
  } else if (!keys[KEY_LALT] && thing) {
    printf("%ld no\n", ms);
    thing = false;
  }
  if (keys[KEY_LMETA] && (keys[KEY_Q] || keys[KEY_W])) {
    quit = true;
  }
  return !quit;
}

void window_closed() {
  printf("window closed\n");
  quit = true;
}