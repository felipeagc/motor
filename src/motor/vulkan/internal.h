#ifndef MT_VULKAN_INTERNAL_H
#define MT_VULKAN_INTERNAL_H

#include <assert.h>
#include "volk.h"

typedef struct MtVulkanWindowSystem {
  const char **(*get_vulkan_instance_extensions)(uint32_t *count);
  int32_t (*get_physical_device_presentation_support)(
      VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);
} MtVulkanWindowSystem;

#define VK_CHECK(exp)                                                          \
  do {                                                                         \
    VkResult result = exp;                                                     \
    assert(result == VK_SUCCESS);                                              \
  } while (0)

#endif
