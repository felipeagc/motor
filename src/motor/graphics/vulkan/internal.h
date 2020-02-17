#pragma once

#include <assert.h>
#include <stdint.h>
#include "volk.h"
#include <motor/base/array.h>
#include <motor/base/hashmap.h>
#include <motor/base/threads.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/vulkan/vulkan_device.h>

enum { FRAMES_IN_FLIGHT = 2 };

#define VK_CHECK(exp)                                                                              \
    do                                                                                             \
    {                                                                                              \
        VkResult result = exp;                                                                     \
        if (result != VK_SUCCESS)                                                                  \
        {                                                                                          \
            mt_log_error("Vulkan result: %u\n", result);                                           \
        }                                                                                          \
        assert(result == VK_SUCCESS);                                                              \
    } while (0)

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

typedef struct QueueFamilyIndices
{
    uint32_t graphics;
    uint32_t transfer;
    uint32_t compute;
} QueueFamilyIndices;

typedef struct BufferBlockAllocation
{
    uint8_t *mapping;
    size_t offset;
    size_t padded_size;
} BufferBlockAllocation;

typedef struct BufferBlock
{
    MtBuffer *buffer;
    size_t offset;
    size_t alignment;
    size_t spill_size;
    size_t size;
    uint8_t *mapping;
} BufferBlock;

typedef struct BufferPool
{
    MtDevice *dev;
    size_t block_size;
    size_t alignment;
    size_t spill_size;
    MtBufferUsage usage;
    /*array*/ BufferBlock *blocks;
} BufferPool;

typedef struct MtDevice
{
    MtAllocator *alloc;

    MtVulkanDeviceFlags flags;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice physical_device;
    VkDevice device;
    MtMutex device_mutex;

    QueueFamilyIndices indices;

    VkQueue graphics_queue;
    VkQueue transfer_queue;
    VkQueue compute_queue;

    VmaAllocator gpu_allocator;

    VkPhysicalDeviceProperties physical_device_properties;

    VkFormat preferred_depth_format;

    uint32_t num_threads;
    VkCommandPool *graphics_cmd_pools;
    VkCommandPool *compute_cmd_pools;
    VkCommandPool *transfer_cmd_pools;

    MtHashMap pipeline_layout_map;

    BufferPool ubo_pool;
    BufferPool vbo_pool;
    BufferPool ibo_pool;
} MtDevice;

typedef struct MtRenderPass
{
    VkRenderPass renderpass;
    VkExtent2D extent;
    VkFramebuffer current_framebuffer;
    uint64_t hash;
    uint32_t color_attachment_count;
    bool has_depth_attachment;
} MtRenderPass;

typedef struct MtSwapchain
{
    MtDevice *dev;
    MtWindow *window;
    MtAllocator *alloc;

    uint32_t present_family_index;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    uint32_t swapchain_image_count;
    uint32_t current_image_index;

    VkFormat swapchain_image_format;
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;

    VkImage depth_image;
    VmaAllocation depth_image_allocation;
    VkImageView depth_image_view;

    uint64_t last_time;
    float delta_time;

    VkQueue present_queue;
    VkExtent2D extent;
} MtSwapchain;

typedef struct SubmitInfo
{
    MtCmdBuffer *cmd_buffer;

    uint32_t wait_semaphore_count;
    VkSemaphore *wait_semaphores;
    const VkPipelineStageFlags *wait_stages; // which stage to execute after waiting

    uint32_t signal_semaphore_count;
    VkSemaphore *signal_semaphores;

    VkFence fence;
} SubmitInfo;

enum {
    MAX_DESCRIPTOR_BINDINGS = 8,
    MAX_DESCRIPTOR_SETS = 8,
    MAX_VERTEX_ATTRIBUTES = 8,
};

typedef struct SetInfo
{
    uint32_t binding_count;
    VkDescriptorSetLayoutBinding bindings[MAX_DESCRIPTOR_BINDINGS];
} SetInfo;

typedef struct VertexAttribute
{
    VkFormat format;
    uint32_t size;
} VertexAttribute;

typedef struct Shader
{
    VkShaderModule mod;
    VkShaderStageFlagBits stage;

    uint32_t set_count;
    uint32_t vertex_attribute_count;

    char *entry_point;

    SetInfo sets[MAX_DESCRIPTOR_SETS];
    VertexAttribute vertex_attributes[MAX_VERTEX_ATTRIBUTES];
} Shader;

typedef union Descriptor
{
    VkDescriptorImageInfo image;
    VkDescriptorBufferInfo buffer;
} Descriptor;

enum { SETS_PER_PAGE = 16 };

typedef struct DescriptorPool
{
    /*array*/ VkDescriptorPool *pools;
    /*array*/ VkDescriptorSet **set_arrays;
    /*array*/ uint32_t *allocated_set_counts;
    /*array*/ MtHashMap *pool_hashmaps;

    VkDescriptorSetLayout set_layout;
    VkDescriptorUpdateTemplate update_template;
    /*array*/ VkDescriptorPoolSize *pool_sizes;
} DescriptorPool;

typedef struct PipelineLayout
{
    VkPipelineLayout layout;
    VkPipelineBindPoint bind_point;

    /*array*/ DescriptorPool *pools;

    SetInfo sets[MAX_DESCRIPTOR_SETS];
    uint32_t set_count;

    uint64_t hash;
    uint32_t ref_count;
} PipelineLayout;

typedef struct PipelineInstance
{
    VkPipeline vk_pipeline;
    MtPipeline *pipeline;
    VkPipelineBindPoint bind_point;
    uint64_t hash;
} PipelineInstance;

typedef struct MtPipeline
{
    VkPipelineBindPoint bind_point;
    MtGraphicsPipelineCreateInfo create_info;
    /*array*/ Shader *shaders;
    uint64_t hash;
    PipelineLayout *layout;
    MtHashMap instances;
} MtPipeline;

typedef struct MtFence
{
    VkFence fence;
} MtFence;

typedef struct MtSemaphore
{
    VkSemaphore semaphore;
} MtSemaphore;

typedef struct MtCmdBuffer
{
    MtDevice *dev;
    VkCommandBuffer cmd_buffer;
    PipelineInstance *bound_pipeline_instance;

    MtRenderPass current_renderpass;

    MtViewport current_viewport;

    uint32_t queue_type;

    Descriptor bound_descriptors[MAX_DESCRIPTOR_BINDINGS][MAX_DESCRIPTOR_SETS];
    uint64_t bound_descriptor_set_hashes[MAX_DESCRIPTOR_SETS];

    BufferBlock ubo_block;
    BufferBlock vbo_block;
    BufferBlock ibo_block;
} MtCmdBuffer;

typedef struct MtBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    size_t size;
    MtBufferUsage usage;
    MtBufferMemory memory;
} MtBuffer;

typedef struct MtImage
{
    VkImage image;
    VmaAllocation allocation;
    VkImageView image_view;

    VkSampleCountFlags sample_count;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_count;
    uint32_t layer_count;
    VkImageAspectFlags aspect;
    VkFormat format;
} MtImage;

typedef struct MtSampler
{
    VkSampler sampler;
} MtSampler;

typedef enum GraphResourceType {
    GRAPH_RESOURCE_IMAGE,
    GRAPH_RESOURCE_BUFFER,
    GRAPH_RESOURCE_EXTERNAL_BUFFER,
} GraphResourceType;

typedef struct GraphResource
{
    GraphResourceType type;
    uint32_t index;

    uint32_t *read_in_passes;
    uint32_t *written_in_passes;

    union
    {
        struct
        {
            MtImageCreateInfo image_info;
            MtImage *image;
        };
        struct
        {
            MtBufferCreateInfo buffer_info;
            MtBuffer *buffer;
        };
    };
} GraphResource;

typedef struct ExecutionGroup
{
    MtQueueType queue_type;
    struct
    {
        MtCmdBuffer *cmd_buffer;
        VkSemaphore execution_finished_semaphore;
        /*array*/ VkSemaphore *wait_semaphores;
        /*array*/ VkPipelineStageFlags *wait_stages;
        VkFence fence;
    } frames[FRAMES_IN_FLIGHT];
    uint32_t *pass_indices;
} ExecutionGroup;

typedef struct MtRenderGraph
{
    MtDevice *dev;
    MtSwapchain *swapchain;
    VkSemaphore image_available_semaphores[FRAMES_IN_FLIGHT];
    void *user_data;

    MtRenderGraphBuilder graph_builder;

    uint32_t current_frame;
    uint32_t frame_count;
    bool framebuffer_resized;

    /*array*/ MtRenderGraphPass *passes;
    /*array*/ GraphResource *resources;

    MtHashMap pass_indices;
    MtHashMap resource_indices;

    /*array*/ VkBufferMemoryBarrier *buffer_barriers;
    /*array*/ VkImageMemoryBarrier *image_barriers;

    /*array*/ ExecutionGroup *execution_groups;
} MtRenderGraph;

typedef struct MtRenderGraphPass
{
    const char *name;
    bool present;
    struct MtRenderGraphPass *next;
    struct MtRenderGraphPass *prev;
    uint32_t index;
    VkPipelineStageFlags stage;
    MtQueueType queue_type;

    MtRenderGraph *graph;
    MtRenderGraphColorClearer color_clearer;
    MtRenderGraphDepthStencilClearer depth_stencil_clearer;
    MtRenderGraphPassBuilder builder;

    uint32_t depth_output;
    /*array*/ uint32_t *color_outputs;
    /*array*/ uint32_t *image_transfer_outputs;
    /*array*/ uint32_t *buffer_writes;

    /*array*/ uint32_t *image_transfer_inputs;
    /*array*/ uint32_t *image_sampled_inputs;
    /*array*/ uint32_t *buffer_reads;

    MtRenderPass render_pass;
    /*array*/ VkFramebuffer *framebuffers;
} MtRenderGraphPass;
