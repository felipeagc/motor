#ifndef MT_VULKAN_UTIL
#define MT_VULKAN_UTIL

#include <assert.h>

#define VK_CHECK(exp)                                                          \
  do {                                                                         \
    VkResult result = exp;                                                     \
    assert(result == VK_SUCCESS);                                              \
  } while (0)

#endif
