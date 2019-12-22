#include "../renderer.h"

#include "../util.h"
#include "device.h"
#include "util.h"
#include "conversions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(NDEBUG)
// Debug mode
#define MT_ENABLE_VALIDATION
#endif

#if !defined(MT_ENABLE_VALIDATION)
static const char *VALIDATION_LAYERS[1] = {
    "VK_LAYER_KHRONOS_validation",
};
static const char *INSTANCE_EXTENSIONS[1] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};
#else
static const char *VALIDATION_LAYERS[0]   = {};
static const char *INSTANCE_EXTENSIONS[0] = {};
#endif

static const char *DEVICE_EXTENSIONS[1] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
    void *p_user_data) {
  if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    printf("Validation layer: %s\n", p_callback_data->pMessage);
  }
  return VK_FALSE;
}

static bool are_indices_complete(MtDevice dev, MtQueueFamilyIndices *indices) {
  return indices->graphics != UINT32_MAX &&
         ((dev->flags & MT_DEVICE_HEADLESS) ||
          indices->present != UINT32_MAX) &&
         indices->transfer != UINT32_MAX && indices->compute != UINT32_MAX;
}

static bool check_validation_layer_support(MtDevice device) {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  VkLayerProperties *available_layers =
      mt_alloc(device->arena, sizeof(VkLayerProperties) * layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

  for (uint32_t l = 0; l < MT_LENGTH(VALIDATION_LAYERS); l++) {
    const char *layer_name = VALIDATION_LAYERS[l];

    bool layer_found = false;
    for (uint32_t a = 0; a < layer_count; a++) {
      VkLayerProperties *layer_properties = &available_layers[a];
      if (strcmp(layer_name, layer_properties->layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      mt_free(device->arena, available_layers);
      return false;
    }
  }

  mt_free(device->arena, available_layers);
  return true;
}

static void create_instance(MtDevice device) {
#ifdef MT_ENABLE_VALIDATION
  if (!check_validation_layer_support(device)) {
    printf("Application wants to enable validation layers but does not support "
           "them\n");
    exit(1);
  }
#endif

  VkApplicationInfo app_info = {
      .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName   = "Motor",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .pEngineName        = "Motor",
      .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
      .apiVersion         = VK_API_VERSION_1_1,
  };

  VkInstanceCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
  };

#ifdef MT_ENABLE_VALIDATION
  create_info.enabledLayerCount   = MT_LENGTH(VALIDATION_LAYERS);
  create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
#endif

  const char **extensions  = NULL;
  uint32_t extension_count = 0;
  if (MT_LENGTH(INSTANCE_EXTENSIONS) > 0) {
    extension_count = MT_LENGTH(INSTANCE_EXTENSIONS);
    extensions      = mt_alloc(device->arena, sizeof(char *) * extension_count);
    memcpy(
        extensions,
        INSTANCE_EXTENSIONS,
        sizeof(char *) * MT_LENGTH(INSTANCE_EXTENSIONS));
  }

  /* if (!headless) { */
  /*   uint32_t glfw_extension_count; */
  /*   const char **glfw_extensions = */
  /*       glfwGetRequiredInstanceExtensions(&glfw_extension_count); */

  /*   extensions = mem.realloc !( */
  /*       char *)(extensions, extensions.length + glfw_extension_count); */

  /*   memcpy( */
  /*       &extensions[extensions.length - glfw_extension_count], */
  /*       glfw_extensions, */
  /*       glfw_extension_count * sizeof(char *)); */
  /* } */

  create_info.enabledExtensionCount   = extension_count;
  create_info.ppEnabledExtensionNames = extensions;

  VK_CHECK(vkCreateInstance(&create_info, NULL, &device->instance));

  volkLoadInstance(device->instance);

  mt_free(device->arena, extensions);
}

static void create_debug_messenger(MtDevice device) {
  VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = &debug_callback;

  VK_CHECK(vkCreateDebugUtilsMessengerEXT(
      device->instance, &create_info, NULL, &device->debug_messenger));
}

MtQueueFamilyIndices
find_queue_families(MtDevice dev, VkPhysicalDevice physical_device) {
  MtQueueFamilyIndices indices;
  indices.graphics = UINT32_MAX;
  indices.present  = UINT32_MAX;
  indices.transfer = UINT32_MAX;
  indices.compute  = UINT32_MAX;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, NULL);

  VkQueueFamilyProperties *queue_families = mt_calloc(
      dev->arena, sizeof(VkQueueFamilyProperties) * queue_family_count);

  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, queue_families);

  for (uint32_t i = 0; i < queue_family_count; i++) {
    VkQueueFamilyProperties *queue_family = &queue_families[i];
    if (queue_family->queueCount > 0 &&
        queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics = i;
    }

    if (queue_family->queueCount > 0 &&
        queue_family->queueFlags & VK_QUEUE_TRANSFER_BIT) {
      indices.transfer = i;
    }

    if (queue_family->queueCount > 0 &&
        queue_family->queueFlags & VK_QUEUE_COMPUTE_BIT) {
      indices.compute = i;
    }

    if (!dev->flags & MT_DEVICE_HEADLESS) {
      /* if (queue_family->queueCount > 0 && */
      /*     glfwGetPhysicalDevicePresentationSupport( */
      /*         instance, physical_device, i)) { */
      /*   indices.present = i; */
      /* } */
    }

    if (are_indices_complete(dev, &indices)) break;
  }

  mt_free(dev->arena, queue_families);
  return indices;
}

bool check_device_extension_support(
    MtDevice dev, VkPhysicalDevice physical_device) {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(
      physical_device, NULL, &extension_count, NULL);

  VkExtensionProperties *available_extensions =
      mt_alloc(dev->arena, sizeof(VkExtensionProperties) * extension_count);

  vkEnumerateDeviceExtensionProperties(
      physical_device, NULL, &extension_count, available_extensions);

  bool found_all = true;

  for (uint32_t i = 0; i < MT_LENGTH(DEVICE_EXTENSIONS); i++) {
    const char *required_ext = DEVICE_EXTENSIONS[i];
    bool found               = false;
    for (uint32_t j = 0; j < extension_count; j++) {
      VkExtensionProperties *available_ext = &available_extensions[j];
      if (strcmp(available_ext->extensionName, required_ext) == 0) {
        found = true;
        break;
      }
    }

    if (!found) found_all = false;
  }

  mt_free(dev->arena, available_extensions);
  return found_all;
}

bool is_device_suitable(MtDevice dev, VkPhysicalDevice physical_device) {
  MtQueueFamilyIndices indices = find_queue_families(dev, physical_device);

  bool extensions_supported =
      check_device_extension_support(dev, physical_device);

  return are_indices_complete(dev, &indices) && extensions_supported;
}

static void pick_physical_device(MtDevice dev) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(dev->instance, &device_count, NULL);

  if (device_count == 0) {
    printf("No vulkan capable devices found\n");
    exit(1);
  }

  VkPhysicalDevice *devices =
      mt_alloc(dev->arena, sizeof(VkPhysicalDevice) * device_count);
  vkEnumeratePhysicalDevices(dev->instance, &device_count, devices);

  for (uint32_t i = 0; i < device_count; i++) {
    if (is_device_suitable(dev, devices[i])) {
      dev->physical_device = devices[i];
      break;
    }
  }

  if (dev->physical_device == VK_NULL_HANDLE) {
    printf("Could not find a physical device that suits the application "
           "requirements\n");
    exit(1);
  }

  vkGetPhysicalDeviceProperties(
      dev->physical_device, &dev->physical_device_properties);

  mt_free(dev->arena, devices);
}

static void create_device(MtDevice dev) {
  dev->indices = find_queue_families(dev, dev->physical_device);

  float queue_priority                          = 1.0f;
  VkDeviceQueueCreateInfo queue_create_infos[4] = {0};
  uint32_t queue_create_info_count              = 1;

  queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_infos[0].queueFamilyIndex = dev->indices.graphics;
  queue_create_infos[0].queueCount       = 1;
  queue_create_infos[0].pQueuePriorities = &queue_priority;

  if (dev->indices.graphics != dev->indices.transfer) {
    queue_create_info_count++;
    queue_create_infos[queue_create_info_count - 1].sType =
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[queue_create_info_count - 1].queueFamilyIndex =
        dev->indices.transfer;
    queue_create_infos[queue_create_info_count - 1].queueCount = 1;
    queue_create_infos[queue_create_info_count - 1].pQueuePriorities =
        &queue_priority;
  }

  if (!dev->flags & MT_DEVICE_HEADLESS) {
    if (dev->indices.graphics != dev->indices.present) {
      queue_create_info_count++;
      queue_create_infos[queue_create_info_count - 1].sType =
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_infos[queue_create_info_count - 1].queueFamilyIndex =
          dev->indices.present;
      queue_create_infos[queue_create_info_count - 1].queueCount = 1;
      queue_create_infos[queue_create_info_count - 1].pQueuePriorities =
          &queue_priority;
    }
  }

  if (dev->indices.graphics != dev->indices.compute) {
    queue_create_info_count++;
    queue_create_infos[queue_create_info_count - 1].sType =
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[queue_create_info_count - 1].queueFamilyIndex =
        dev->indices.compute;
    queue_create_infos[queue_create_info_count - 1].queueCount = 1;
    queue_create_infos[queue_create_info_count - 1].pQueuePriorities =
        &queue_priority;
  }

  VkPhysicalDeviceFeatures device_features = {
      .samplerAnisotropy = VK_TRUE,
  };

  VkDeviceCreateInfo create_info = {
      .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pQueueCreateInfos    = queue_create_infos,
      .queueCreateInfoCount = queue_create_info_count,
      .pEnabledFeatures     = &device_features,
  };

#ifdef MT_ENABLE_VALIDATION
  create_info.enabledLayerCount   = MT_LENGTH(VALIDATION_LAYERS);
  create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
#endif

  if (!dev->flags & MT_DEVICE_HEADLESS) {
    create_info.enabledExtensionCount   = MT_LENGTH(DEVICE_EXTENSIONS);
    create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS;
  }

  VK_CHECK(
      vkCreateDevice(dev->physical_device, &create_info, NULL, &dev->device));

  vkGetDeviceQueue(dev->device, dev->indices.graphics, 0, &dev->graphics_queue);
  vkGetDeviceQueue(dev->device, dev->indices.transfer, 0, &dev->transfer_queue);

  if (!dev->flags & MT_DEVICE_HEADLESS) {
    vkGetDeviceQueue(dev->device, dev->indices.present, 0, &dev->present_queue);
  }

  vkGetDeviceQueue(dev->device, dev->indices.compute, 0, &dev->compute_queue);
}

static void create_allocator(MtDevice dev) {
  VmaAllocatorCreateInfo allocator_info = {0};
  allocator_info.physicalDevice         = dev->physical_device;
  allocator_info.device                 = dev->device;

  VmaVulkanFunctions vk_functions = {
      .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
      .vkGetPhysicalDeviceMemoryProperties =
          vkGetPhysicalDeviceMemoryProperties,
      .vkAllocateMemory               = vkAllocateMemory,
      .vkFreeMemory                   = vkFreeMemory,
      .vkMapMemory                    = vkMapMemory,
      .vkUnmapMemory                  = vkUnmapMemory,
      .vkFlushMappedMemoryRanges      = vkFlushMappedMemoryRanges,
      .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
      .vkBindBufferMemory             = vkBindBufferMemory,
      .vkBindImageMemory              = vkBindImageMemory,
      .vkGetBufferMemoryRequirements  = vkGetBufferMemoryRequirements,
      .vkGetImageMemoryRequirements   = vkGetImageMemoryRequirements,
      .vkCreateBuffer                 = vkCreateBuffer,
      .vkDestroyBuffer                = vkDestroyBuffer,
      .vkCreateImage                  = vkCreateImage,
      .vkDestroyImage                 = vkDestroyImage,
      .vkCmdCopyBuffer                = vkCmdCopyBuffer,
  };

  allocator_info.pVulkanFunctions = &vk_functions;

  VK_CHECK(vmaCreateAllocator(&allocator_info, &dev->gpu_allocator));
}

static VkFormat find_supported_format(
    MtDevice dev,
    VkFormat *candidates,
    uint32_t candidate_count,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) {
  for (uint32_t i = 0; i < candidate_count; i++) {
    VkFormat format = candidates[i];

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(dev->physical_device, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (
        tiling == VK_IMAGE_TILING_OPTIMAL &&
        (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  return VK_FORMAT_UNDEFINED;
}

static void find_supported_depth_format(MtDevice dev) {
  VkFormat candidates[3] = {
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D32_SFLOAT,
  };

  dev->preferred_depth_format = find_supported_format(
      dev,
      candidates,
      MT_LENGTH(candidates),
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

MtDevice mt_device_create(MtArena *arena, MtDeviceFlags flags) {
  MtDevice dev = mt_calloc(arena, sizeof(struct MtDevice_T));
  dev->flags   = flags;
  dev->arena   = arena;

  VK_CHECK(volkInitialize());

  create_instance(dev);

#ifdef MT_ENABLE_VALIDATION
  create_debug_messenger(dev);
#endif

  pick_physical_device(dev);
  create_device(dev);
  create_allocator(dev);

  find_supported_depth_format(dev);

  return dev;
}

void mt_device_destroy(MtDevice dev) {
  MtArena *arena = dev->arena;
  vkDeviceWaitIdle(dev->device);

  vmaDestroyAllocator(dev->gpu_allocator);

  vkDestroyDevice(dev->device, NULL);

#ifdef MT_ENABLE_VALIDATION
  vkDestroyDebugUtilsMessengerEXT(dev->instance, dev->debug_messenger, NULL);
#endif

  vkDestroyInstance(dev->instance, NULL);

  mt_free(arena, dev);
}
