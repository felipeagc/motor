#ifndef MT_VULKAN_INTERNAL_H
#define MT_VULKAN_INTERNAL_H

#include <assert.h>
#include <stdint.h>
#include "volk.h"
#include "../../../include/motor/array.h"
#include "../../../include/motor/hashmap.h"
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/vulkan/vulkan_device.h"

enum { FRAMES_IN_FLIGHT = 2 };

#define VK_CHECK(exp)                                                          \
  do {                                                                         \
    VkResult result = exp;                                                     \
    assert(result == VK_SUCCESS);                                              \
  } while (0)

VK_DEFINE_HANDLE(VmaAllocator)

typedef struct QueueFamilyIndices {
  uint32_t graphics;
  uint32_t present;
  uint32_t transfer;
  uint32_t compute;
} QueueFamilyIndices;

typedef struct MtWindowSystem {
  const char **(*get_vulkan_instance_extensions)(uint32_t *count);
  int32_t (*get_physical_device_presentation_support)(
      VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);
} MtWindowSystem;

typedef struct MtDevice {
  MtArena *arena;

  MtVulkanDeviceFlags flags;
  MtWindowSystem *window_system;

  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;

  VkPhysicalDevice physical_device;
  VkDevice device;

  QueueFamilyIndices indices;

  VkQueue graphics_queue;
  VkQueue transfer_queue;
  VkQueue present_queue;
  VkQueue compute_queue;

  VmaAllocator gpu_allocator;

  VkPhysicalDeviceProperties physical_device_properties;

  VkFormat preferred_depth_format;

  uint32_t num_threads;
  VkCommandPool *graphics_cmd_pools;
  VkCommandPool *compute_cmd_pools;
  VkCommandPool *transfer_cmd_pools;

  MtHashMap descriptor_set_allocators;
  MtHashMap pipeline_layout_map;
  MtHashMap pipeline_map;
} MtDevice;

typedef struct MtRenderPass {
  VkRenderPass renderpass;
  VkExtent2D extent;
  VkFramebuffer current_framebuffer;
  uint64_t hash;
} MtRenderPass;

typedef struct SetInfo {
  uint32_t index;
  /*array*/ VkDescriptorSetLayoutBinding *bindings;
} SetInfo;

typedef struct Shader {
  VkShaderModule mod;
  VkShaderStageFlagBits stage;

  /*array*/ VkPushConstantRange *push_constants;
  /*array*/ SetInfo *sets;
} Shader;

typedef union Descriptor {
  VkDescriptorImageInfo image;
  VkDescriptorBufferInfo buffer;
} Descriptor;

typedef struct PipelineLayout {
  VkPipelineLayout layout;
  /* DSAllocator *set_allocator; */
  VkPipelineBindPoint bind_point;

  /*array*/ VkDescriptorUpdateTemplate *update_templates;
  /*array*/ VkDescriptorSetLayout *set_layouts;

  /*array*/ SetInfo *sets;
  /*array*/ VkPushConstantRange *push_constants;
} PipelineLayout;

typedef struct PipelineBundle {
  VkPipeline pipeline;
  PipelineLayout *layout;
  VkPipelineBindPoint bind_point;
} PipelineBundle;

typedef struct MtPipeline {
  VkPipelineBindPoint bind_point;
  MtGraphicsPipelineDescriptor descriptor;
  /*array*/ Shader *shaders;
  uint64_t hash;
} MtPipeline;

typedef struct MtCmdBuffer {
  MtDevice *dev;
  VkCommandBuffer cmd_buffer;
  PipelineBundle *bound_pipeline;
  MtRenderPass current_renderpass;
  uint32_t queue_type;
} MtCmdBuffer;

#endif
