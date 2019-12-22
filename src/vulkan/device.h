#ifndef MT_VULKAN_DEVICE_H
#define MT_VULKAN_DEVICE_H

#include "../arena.h"
#include "volk.h"
#include "vk_mem_alloc.h"

typedef struct MtQueueFamilyIndices {
  uint32_t graphics;
  uint32_t present;
  uint32_t transfer;
  uint32_t compute;
} MtQueueFamilyIndices;

struct MtDevice_T {
  MtArena *arena;

  MtDeviceFlags flags;

  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;

  VkPhysicalDevice physical_device;
  VkDevice device;

  MtQueueFamilyIndices indices;

  VkQueue graphics_queue;
  VkQueue transfer_queue;
  VkQueue present_queue;
  VkQueue compute_queue;

  VmaAllocator gpu_allocator;

  VkPhysicalDeviceProperties physical_device_properties;

  VkFormat preferred_depth_format;
};

#endif
