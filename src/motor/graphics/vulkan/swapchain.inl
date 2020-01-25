/*
 * Setup functions
 */

#include <motor/base/time.h>

// clang-format off
#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
#elif defined(__APPLE__)
    #error Apple not supported
#else
    #include <xcb/xcb.h>
    #include <X11/Xlib-xcb.h>
    #include <vulkan/vulkan_xcb.h>
#endif
// clang-format on

#define clamp(v, a, b) ((((v > b) ? b : v) < a) ? a : v)

typedef struct SwapchainSupport
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t format_count;
    VkPresentModeKHR *present_modes;
    uint32_t present_mode_count;
} SwapchainSupport;

static const char **swapchain_get_required_instance_extensions(uint32_t *count)
{
#if defined(_WIN32)
    static const char *extensions[2];
    extensions[0] = "VK_KHR_surface";
    extensions[1] = "VK_KHR_win32_surface";
    *count        = 2;
    return extensions;
#elif defined(__APPLE__)
#error Apple not supported
#else
    static const char *extensions[2];
    extensions[0] = "VK_KHR_surface";
    extensions[1] = "VK_KHR_xcb_surface";
    *count        = 2;
    return extensions;
#endif
}

static void swapchain_create_surface(MtDevice *dev, MtWindow *window, VkSurfaceKHR *surface)
{
#if defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR sci = {0};
    sci.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hinstance                   = GetModuleHandle(NULL);
    sci.hwnd                        = mt_window.get_win32_display(window);
    VK_CHECK(vkCreateWin32SurfaceKHR(dev->instance, &sci, NULL, surface));
#elif defined(__APPLE__)
#error Apple not supported
#else
    xcb_connection_t *connection = XGetXCBConnection(mt_window.get_x11_display(window));
    if (!connection)
    {
        printf("Failed to get XCB connection\n");
        exit(1);
    }

    PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR =
        (PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(dev->instance, "vkCreateXcbSurfaceKHR");
    if (!vkCreateXcbSurfaceKHR)
    {
        printf("Failed to get vkCreateXcbSurfaceKHR function pointer\n");
        exit(1);
    }

    VkXcbSurfaceCreateInfoKHR sci = {0};
    sci.sType                     = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    sci.connection                = connection;
    sci.window                    = mt_window.get_x11_window(window);
    VK_CHECK(vkCreateXcbSurfaceKHR(dev->instance, &sci, NULL, surface));
#endif
}

static void swapchain_create_semaphores(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        VK_CHECK(vkCreateSemaphore(
            dev->device, &semaphore_info, NULL, &swapchain->image_available_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(
            dev->device, &semaphore_info, NULL, &swapchain->render_finished_semaphores[i]));
    }
}

static void swapchain_create_fences(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        VK_CHECK(vkCreateFence(dev->device, &fence_info, NULL, &swapchain->in_flight_fences[i]));
    }
}

static SwapchainSupport swapchain_query_swapchain_support(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    SwapchainSupport details = {0};

    // Capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        dev->physical_device, swapchain->surface, &details.capabilities);

    // Formats
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        dev->physical_device, swapchain->surface, &details.format_count, NULL);
    if (details.format_count != 0)
    {
        details.formats =
            mt_alloc(swapchain->alloc, sizeof(VkSurfaceFormatKHR) * details.format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            dev->physical_device, swapchain->surface, &details.format_count, details.formats);
    }

    // Present modes
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        dev->physical_device, swapchain->surface, &details.present_mode_count, NULL);
    if (details.present_mode_count != 0)
    {
        details.present_modes =
            mt_alloc(swapchain->alloc, sizeof(VkPresentModeKHR) * details.present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            dev->physical_device,
            swapchain->surface,
            &details.present_mode_count,
            details.present_modes);
    }

    return details;
}

static VkSurfaceFormatKHR
swapchain_choose_swapchain_surface_format(VkSurfaceFormatKHR *formats, uint32_t count)
{
    if (count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        VkSurfaceFormatKHR fmt = {
            .format     = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = formats[0].colorSpace,
        };

        return fmt;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        VkSurfaceFormatKHR surface_format = formats[i];

        // Prefer SRGB
        if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            return surface_format;
        }
    }

    return formats[0];
}

static VkPresentModeKHR
swapchain_choose_swapchain_present_mode(VkPresentModeKHR *present_modes, uint32_t count)
{
    /* for (uint32_t i = 0; i < count; i++) */
    /* { */
    /*     VkPresentModeKHR mode = present_modes[i]; */
    /*     if (mode == VK_PRESENT_MODE_FIFO_KHR) */
    /*     { */
    /*         return mode; */
    /*     } */
    /* } */

    for (uint32_t i = 0; i < count; i++)
    {
        VkPresentModeKHR mode = present_modes[i];
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            return mode;
        }
    }

    for (uint32_t i = 0; i < count; i++)
    {
        VkPresentModeKHR mode = present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return mode;
        }
    }

    for (uint32_t i = 0; i < count; i++)
    {
        VkPresentModeKHR mode = present_modes[i];
        if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
        {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D
swapchain_choose_swapchain_extent(MtSwapchain *swapchain, VkSurfaceCapabilitiesKHR capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        uint32_t width = 0, height = 0;
        mt_window.get_size(swapchain->window, &width, &height);
        VkExtent2D actual_extent = {width, height};

        actual_extent.width = clamp(
            actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);

        actual_extent.width = clamp(
            actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);

        return actual_extent;
    }
}

static void swapchain_create_swapchain(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    SwapchainSupport swapchain_support = swapchain_query_swapchain_support(swapchain);

    if (swapchain_support.format_count == 0 || swapchain_support.present_mode_count == 0)
    {
        printf("Physical device does not support swapchain creation\n");
        exit(1);
    }

    VkSurfaceFormatKHR surface_format = swapchain_choose_swapchain_surface_format(
        swapchain_support.formats, swapchain_support.format_count);
    VkPresentModeKHR present_mode = swapchain_choose_swapchain_present_mode(
        swapchain_support.present_modes, swapchain_support.present_mode_count);
    VkExtent2D extent =
        swapchain_choose_swapchain_extent(swapchain, swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

    if (swapchain_support.capabilities.maxImageCount > 0 &&
        image_count > swapchain_support.capabilities.maxImageCount)
    {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkImageUsageFlags image_usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (!(swapchain_support.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
    {
        printf("Physical device does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT "
               "in swapchains\n");
        exit(1);
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = swapchain->surface,
        .minImageCount    = image_count,
        .imageFormat      = surface_format.format,
        .imageColorSpace  = surface_format.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = image_usage,
    };

    uint32_t queue_family_indices[2] = {dev->indices.graphics, swapchain->present_family_index};

    if (dev->indices.graphics != swapchain->present_family_index)
    {
        create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices   = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices   = NULL;
    }

    create_info.preTransform   = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    create_info.presentMode = present_mode;
    create_info.clipped     = VK_TRUE;

    VkSwapchainKHR old_swapchain = swapchain->swapchain;
    create_info.oldSwapchain     = old_swapchain;

    VK_CHECK(vkCreateSwapchainKHR(dev->device, &create_info, NULL, &swapchain->swapchain));

    if (old_swapchain)
    {
        vkDestroySwapchainKHR(dev->device, old_swapchain, NULL);
    }

    vkGetSwapchainImagesKHR(dev->device, swapchain->swapchain, &image_count, NULL);
    swapchain->swapchain_images =
        mt_realloc(swapchain->alloc, swapchain->swapchain_images, sizeof(VkImage) * image_count);
    vkGetSwapchainImagesKHR(
        dev->device, swapchain->swapchain, &image_count, swapchain->swapchain_images);

    swapchain->swapchain_image_count  = image_count;
    swapchain->swapchain_image_format = surface_format.format;
    swapchain->render_pass.extent     = extent;

    mt_free(swapchain->alloc, swapchain_support.formats);
    mt_free(swapchain->alloc, swapchain_support.present_modes);
}

static void swapchain_create_swapchain_image_views(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    swapchain->swapchain_image_views = mt_realloc(
        swapchain->alloc,
        swapchain->swapchain_image_views,
        sizeof(VkImageView) * swapchain->swapchain_image_count);

    for (size_t i = 0; i < swapchain->swapchain_image_count; i++)
    {
        VkImageViewCreateInfo create_info = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image                           = swapchain->swapchain_images[i],
            .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
            .format                          = swapchain->swapchain_image_format,
            .components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1,
        };

        VK_CHECK(vkCreateImageView(
            dev->device, &create_info, NULL, &swapchain->swapchain_image_views[i]));
    }
}

static void swapchain_create_depth_images(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    VkImageCreateInfo image_create_info = {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format    = dev->preferred_depth_format,
        .extent =
            {
                .width  = swapchain->render_pass.extent.width,
                .height = swapchain->render_pass.extent.height,
                .depth  = 1,
            },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo image_alloc_create_info = {0};
    image_alloc_create_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        dev->gpu_allocator,
        &image_create_info,
        &image_alloc_create_info,
        &swapchain->depth_image,
        &swapchain->depth_image_allocation,
        NULL));

    VkImageViewCreateInfo create_info = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image                           = swapchain->depth_image,
        .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
        .format                          = dev->preferred_depth_format,
        .components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
    };

    VK_CHECK(vkCreateImageView(dev->device, &create_info, NULL, &swapchain->depth_image_view));
}

static void swapchain_create_renderpass(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    VkAttachmentDescription color_attachment = {
        .format         = swapchain->swapchain_image_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depth_attachment = {
        .format         = dev->preferred_depth_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref,
    };

    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};

    VkRenderPassCreateInfo renderpass_create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = MT_LENGTH(attachments),
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };

    VK_CHECK(vkCreateRenderPass(
        dev->device, &renderpass_create_info, NULL, &swapchain->render_pass.renderpass));

    swapchain->render_pass.hash = vulkan_hash_render_pass(&renderpass_create_info);
}

static void swapchain_create_framebuffers(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    swapchain->swapchain_framebuffers = mt_realloc(
        swapchain->alloc,
        swapchain->swapchain_framebuffers,
        sizeof(VkFramebuffer) * swapchain->swapchain_image_count);

    for (size_t i = 0; i < swapchain->swapchain_image_count; i++)
    {
        VkImageView attachments[2] = {swapchain->swapchain_image_views[i],
                                      swapchain->depth_image_view};

        VkFramebufferCreateInfo create_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = swapchain->render_pass.renderpass,
            .attachmentCount = MT_LENGTH(attachments),
            .pAttachments    = attachments,
            .width           = swapchain->render_pass.extent.width,
            .height          = swapchain->render_pass.extent.height,
            .layers          = 1,
        };

        VK_CHECK(vkCreateFramebuffer(
            dev->device, &create_info, NULL, &swapchain->swapchain_framebuffers[i]));
    }
}

static void swapchain_create_resizables(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    uint32_t width = 0, height = 0;
    while (width == 0 || height == 0)
    {
        mt_window.get_size(swapchain->window, &width, &height);
        mt_window.wait_events(swapchain->window);
    }

    mt_mutex_lock(&dev->device_mutex);
    VK_CHECK(vkDeviceWaitIdle(dev->device));
    mt_mutex_unlock(&dev->device_mutex);

    swapchain_create_swapchain(swapchain);
    swapchain_create_swapchain_image_views(swapchain);
    swapchain_create_depth_images(swapchain);
    swapchain_create_renderpass(swapchain);
    swapchain_create_framebuffers(swapchain);
}

static void swapchain_destroy_resizables(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    mt_mutex_lock(&dev->device_mutex);
    VK_CHECK(vkDeviceWaitIdle(dev->device));
    mt_mutex_unlock(&dev->device_mutex);

    for (uint32_t i = 0; i < swapchain->swapchain_image_count; i++)
    {
        VkFramebuffer *framebuffer = &swapchain->swapchain_framebuffers[i];
        if (framebuffer)
        {
            vkDestroyFramebuffer(dev->device, *framebuffer, NULL);
            *framebuffer = VK_NULL_HANDLE;
        }
    }

    if (swapchain->render_pass.renderpass)
    {
        vkDestroyRenderPass(dev->device, swapchain->render_pass.renderpass, NULL);
        swapchain->render_pass.renderpass = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < swapchain->swapchain_image_count; i++)
    {
        VkImageView *image_view = &swapchain->swapchain_image_views[i];
        if (image_view)
        {
            vkDestroyImageView(dev->device, *image_view, NULL);
            *image_view = VK_NULL_HANDLE;
        }
    }

    if (swapchain->depth_image)
    {
        vmaDestroyImage(
            dev->gpu_allocator, swapchain->depth_image, swapchain->depth_image_allocation);
        vkDestroyImageView(dev->device, swapchain->depth_image_view, NULL);
    }

    if (swapchain->swapchain)
    {
        vkDestroySwapchainKHR(dev->device, swapchain->swapchain, NULL);
        swapchain->swapchain = VK_NULL_HANDLE;
    }
}

static void swapchain_allocate_cmd_buffers(MtSwapchain *swapchain)
{
    mt_render.allocate_cmd_buffers(
        swapchain->dev, MT_QUEUE_GRAPHICS, FRAMES_IN_FLIGHT, swapchain->cmd_buffers);
}

static void swapchain_free_cmd_buffers(MtSwapchain *swapchain)
{
    mt_render.free_cmd_buffers(
        swapchain->dev, MT_QUEUE_GRAPHICS, FRAMES_IN_FLIGHT, swapchain->cmd_buffers);
}

/*
 * Swapchain functions
 */

static MtCmdBuffer *swapchain_begin_frame(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    vkWaitForFences(
        dev->device,
        1,
        &swapchain->in_flight_fences[swapchain->current_frame],
        VK_TRUE,
        UINT64_MAX);

    VkResult res = vkAcquireNextImageKHR(
        dev->device,
        swapchain->swapchain,
        UINT64_MAX,
        swapchain->image_available_semaphores[swapchain->current_frame],
        VK_NULL_HANDLE,
        &swapchain->current_image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        swapchain_destroy_resizables(swapchain);
        swapchain_create_resizables(swapchain);
        return swapchain_begin_frame(swapchain);
    }
    else
    {
        VK_CHECK(res);
    }

    swapchain->render_pass.current_framebuffer =
        swapchain->swapchain_framebuffers[swapchain->current_image_index];

    return swapchain->cmd_buffers[swapchain->current_frame];
}

static void swapchain_end_frame(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    VkSubmitInfo submit_info = {0};
    submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[1] = {
        swapchain->image_available_semaphores[swapchain->current_frame]};
    VkSemaphore signal_semaphores[1] = {
        swapchain->render_finished_semaphores[swapchain->current_frame]};

    submit_info.waitSemaphoreCount = MT_LENGTH(wait_semaphores);
    submit_info.pWaitSemaphores    = wait_semaphores;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask   = &wait_stage;

    MtCmdBuffer *cmd_buffer = swapchain->cmd_buffers[swapchain->current_frame];

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &cmd_buffer->cmd_buffer;

    submit_info.signalSemaphoreCount = MT_LENGTH(signal_semaphores);
    submit_info.pSignalSemaphores    = signal_semaphores;

    vkResetFences(dev->device, 1, &swapchain->in_flight_fences[swapchain->current_frame]);
    VK_CHECK(vkQueueSubmit(
        dev->graphics_queue,
        1,
        &submit_info,
        swapchain->in_flight_fences[swapchain->current_frame]));

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = MT_LENGTH(signal_semaphores),
        .pWaitSemaphores    = signal_semaphores,
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain->swapchain,
        .pImageIndices      = &swapchain->current_image_index,
    };

    VkResult res = vkQueuePresentKHR(swapchain->present_queue, &present_info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR ||
        swapchain->framebuffer_resized)
    {
        swapchain->framebuffer_resized = false;
        swapchain_destroy_resizables(swapchain);
        swapchain_create_resizables(swapchain);
    }
    else
    {
        VK_CHECK(res);
    }

    swapchain->current_frame = (swapchain->current_frame + 1) % FRAMES_IN_FLIGHT;

    swapchain->delta_time = (float)(mt_time_ns() - swapchain->last_time) / 1.0e9;
    swapchain->last_time  = mt_time_ns();
}

static MtSwapchain *create_swapchain(MtDevice *dev, MtWindow *window, MtAllocator *alloc)
{
    MtSwapchain *swapchain = mt_alloc(alloc, sizeof(MtSwapchain));
    memset(swapchain, 0, sizeof(*swapchain));

    swapchain->alloc  = alloc;
    swapchain->dev    = dev;
    swapchain->window = window;

    swapchain->present_family_index = UINT32_MAX;

    swapchain_create_surface(dev, window, &swapchain->surface);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev->physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_families =
        mt_alloc(dev->alloc, sizeof(VkQueueFamilyProperties) * queue_family_count);
    memset(queue_families, 0, sizeof(VkQueueFamilyProperties) * queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(
        dev->physical_device, &queue_family_count, queue_families);

    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        VkQueueFamilyProperties *queue_family = &queue_families[i];

        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            dev->physical_device, i, swapchain->surface, &supported);

        if (queue_family->queueCount > 0 && supported)
        {
            swapchain->present_family_index = i;
            break;
        }
    }

    mt_free(dev->alloc, queue_families);

    if (swapchain->present_family_index == UINT32_MAX)
    {
        printf("Could not obtain a present queue family.\n");
        exit(1);
    }

    // Get present queue
    vkGetDeviceQueue(dev->device, swapchain->present_family_index, 0, &swapchain->present_queue);

    swapchain_create_semaphores(swapchain);
    swapchain_create_fences(swapchain);

    swapchain_create_resizables(swapchain);

    swapchain_allocate_cmd_buffers(swapchain);

    return swapchain;
}

static void destroy_swapchain(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    mt_mutex_lock(&dev->device_mutex);
    VK_CHECK(vkDeviceWaitIdle(dev->device));
    mt_mutex_unlock(&dev->device_mutex);

    swapchain_free_cmd_buffers(swapchain);

    swapchain_destroy_resizables(swapchain);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(dev->device, swapchain->image_available_semaphores[i], NULL);
        vkDestroySemaphore(dev->device, swapchain->render_finished_semaphores[i], NULL);
        vkDestroyFence(dev->device, swapchain->in_flight_fences[i], NULL);
    }

    vkDestroySurfaceKHR(dev->instance, swapchain->surface, NULL);
}

static MtRenderPass *swapchain_get_render_pass(MtSwapchain *swapchain)
{
    return &swapchain->render_pass;
}

static float swapchain_get_delta_time(MtSwapchain *swapchain)
{
    return swapchain->delta_time;
}
