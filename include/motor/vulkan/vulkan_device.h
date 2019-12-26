#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct MtArena MtArena;
typedef struct MtIDevice MtIDevice;
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

void mt_vulkan_device_init(
    MtIDevice *device, MtVulkanDeviceCreateInfo *descriptor, MtArena *arena);
