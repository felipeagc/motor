#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct MtAllocator MtAllocator;
typedef struct MtDevice MtDevice;
typedef struct MtIWindowSystem MtIWindowSystem;

typedef uint32_t MtVulkanDeviceFlags;
typedef enum MtVulkanDeviceFlagBits {
    MT_DEVICE_HEADLESS = 1,
} MtVulkanDeviceFlagBits;

typedef struct MtVulkanDeviceCreateInfo {
    MtVulkanDeviceFlags flags;
    uint32_t num_threads;
    MtIWindowSystem *window_system;
} MtVulkanDeviceCreateInfo;

MtDevice *
mt_vulkan_device_init(MtVulkanDeviceCreateInfo *descriptor, MtAllocator *alloc);
