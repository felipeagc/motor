#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct MtAllocator MtAllocator;
typedef struct MtDevice MtDevice;

typedef enum MtVulkanDeviceFlags
{
    MT_DEVICE_HEADLESS = 1,
} MtVulkanDeviceFlags;

typedef struct MtVulkanDeviceCreateInfo
{
    MtVulkanDeviceFlags flags;
    uint32_t num_threads;
} MtVulkanDeviceCreateInfo;

MtDevice *mt_vulkan_device_init(MtVulkanDeviceCreateInfo *create_info, MtAllocator *alloc);
