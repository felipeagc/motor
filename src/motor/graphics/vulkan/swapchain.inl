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
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR =
        (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(
            dev->instance, "vkCreateWin32SurfaceKHR");
    if (!vkCreateWin32SurfaceKHR)
    {
        printf("Failed to get vkCreateXcbSurfaceKHR function pointer\n");
        exit(1);
    }

    VkWin32SurfaceCreateInfoKHR sci = {0};
    sci.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hinstance                   = GetModuleHandle(NULL);
    sci.hwnd                        = mt_window.get_win32_window(window);
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
    swapchain->swapchain_extent       = extent;

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

static void swapchain_create_resizables(MtSwapchain *swapchain)
{
    mt_log("Creating resizables");
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
}

static void swapchain_destroy_resizables(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    mt_mutex_lock(&dev->device_mutex);
    VK_CHECK(vkDeviceWaitIdle(dev->device));
    mt_mutex_unlock(&dev->device_mutex);

    for (uint32_t i = 0; i < swapchain->swapchain_image_count; i++)
    {
        VkImageView *image_view = &swapchain->swapchain_image_views[i];
        if (image_view)
        {
            vkDestroyImageView(dev->device, *image_view, NULL);
            *image_view = VK_NULL_HANDLE;
        }
    }

    if (swapchain->swapchain)
    {
        vkDestroySwapchainKHR(dev->device, swapchain->swapchain, NULL);
        swapchain->swapchain = VK_NULL_HANDLE;
    }
}

/*
 * Swapchain functions
 */

static void swapchain_end_frame(MtSwapchain *swapchain)
{
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

    swapchain_create_resizables(swapchain);

    return swapchain;
}

static void destroy_swapchain(MtSwapchain *swapchain)
{
    MtDevice *dev = swapchain->dev;

    mt_mutex_lock(&dev->device_mutex);
    VK_CHECK(vkDeviceWaitIdle(dev->device));
    mt_mutex_unlock(&dev->device_mutex);

    swapchain_destroy_resizables(swapchain);

    vkDestroySurfaceKHR(dev->instance, swapchain->surface, NULL);

    mt_free(dev->alloc, swapchain->swapchain_images);
    mt_free(dev->alloc, swapchain->swapchain_image_views);

    mt_free(dev->alloc, swapchain);
}

static float swapchain_get_delta_time(MtSwapchain *swapchain)
{
    return swapchain->delta_time;
}
