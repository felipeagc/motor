#pragma once

#include <assert.h>
#include <stdint.h>
#include "volk.h"
#include "../../../include/motor/array.h"
#include "../../../include/motor/hashmap.h"
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/bitset.h"
#include "../../../include/motor/vulkan/vulkan_device.h"

enum { FRAMES_IN_FLIGHT = 2 };

#define VK_CHECK(exp)                                                          \
    do {                                                                       \
        VkResult result = exp;                                                 \
        assert(result == VK_SUCCESS);                                          \
    } while (0)

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

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

typedef struct BufferAllocatorPage {
    MtBuffer *buffer;
    MtDynamicBitset in_use;
    size_t part_size;
    struct BufferAllocatorPage *next;
    void *mapping;
    uint32_t last_index;
} BufferAllocatorPage;

typedef struct BufferAllocator {
    MtDevice *dev;
    uint32_t current_frame;
    size_t page_size;
    BufferAllocatorPage base_pages[FRAMES_IN_FLIGHT];
    MtBufferUsage usage;
} BufferAllocator;

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

    BufferAllocator ubo_allocator;
    BufferAllocator vbo_allocator;
    BufferAllocator ibo_allocator;
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

enum { SETS_PER_PAGE = 32 };

typedef struct DSAllocatorPage {
    struct DSAllocatorPage *next;
    VkDescriptorPool pool;

    MtHashMap hashmap;
    /* array */ VkDescriptorSet *sets;
    /* array */ uint64_t *hashes;
    /* array */ uint32_t *set_ages;
    MT_BITSET(SETS_PER_PAGE) in_use;

    uint32_t set_index;
    uint32_t last_index;
} DSAllocatorPage;

typedef struct DSAllocatorFrame {
    /* array */ DSAllocatorPage *base_pages; // [set_index]
    /* array */ DSAllocatorPage **last_pages;
} DSAllocatorFrame;

typedef struct DSAllocator {
    MtDevice *dev;
    uint32_t current_frame;

    DSAllocatorFrame frames[FRAMES_IN_FLIGHT];

    VkDescriptorSetLayout *set_layouts;           // not owned by this
    VkDescriptorUpdateTemplate *update_templates; // not owned by this
    VkDescriptorPoolSize **pool_sizes;            // [pool_size][set]
} DSAllocator;

typedef struct PipelineLayout {
    VkPipelineLayout layout;
    DSAllocator *set_allocator;
    VkPipelineBindPoint bind_point;

    /*array*/ VkDescriptorUpdateTemplate *update_templates;
    /*array*/ VkDescriptorSetLayout *set_layouts;

    /*array*/ SetInfo *sets;
    /*array*/ VkPushConstantRange *push_constants;
} PipelineLayout;

typedef struct PipelineInstance {
    VkPipeline pipeline;
    PipelineLayout *layout;
    VkPipelineBindPoint bind_point;
} PipelineInstance;

typedef struct MtPipeline {
    VkPipelineBindPoint bind_point;
    MtGraphicsPipelineCreateInfo create_info;
    /*array*/ Shader *shaders;
    uint64_t hash;
} MtPipeline;

enum { MAX_DESCRIPTOR_BINDINGS = 8 };
enum { MAX_DESCRIPTOR_SETS = 8 };

typedef struct MtCmdBuffer {
    MtDevice *dev;
    VkCommandBuffer cmd_buffer;
    PipelineInstance *bound_pipeline;
    MtRenderPass current_renderpass;
    uint32_t queue_type;
    Descriptor bound_descriptors[MAX_DESCRIPTOR_BINDINGS][MAX_DESCRIPTOR_SETS];
} MtCmdBuffer;

typedef struct MtBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    size_t size;
    MtBufferUsage usage;
    MtBufferMemory memory;
} MtBuffer;
