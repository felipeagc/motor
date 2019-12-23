#ifndef MT_VULKAN_INTERNAL_H
#define MT_VULKAN_INTERNAL_H

#include <assert.h>
#include <stdint.h>
#include "volk.h"
#include "../../../include/motor/vulkan/vulkan_device.h"

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

typedef struct VulkanWindowSystem {
  const char **(*get_vulkan_instance_extensions)(uint32_t *count);
  int32_t (*get_physical_device_presentation_support)(
      VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);
} VulkanWindowSystem;

typedef struct VulkanDevice {
  MtArena *arena;

  MtVulkanDeviceFlags flags;
  VulkanWindowSystem *window_system;

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
} VulkanDevice;

#endif
