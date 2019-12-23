#include "../../../include/motor/vulkan/glfw_window.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "internal.h"
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"
#include "hashing.h"
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/window.h"
#include "../../../include/motor/util.h"

#define clamp(v, a, b) ((((v > b) ? b : v) < a) ? a : v)

enum { FRAMES_IN_FLIGHT = 2 };

/*
 * Window
 */

typedef struct MtWindow {
  MtIDevice dev;
  MtArena *arena;

  GLFWwindow *window;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  VkSemaphore image_available_semaphores[FRAMES_IN_FLIGHT];
  VkSemaphore render_finished_semaphores[FRAMES_IN_FLIGHT];
  VkFence in_flight_fences[FRAMES_IN_FLIGHT];

  uint32_t swapchain_image_count;
  uint32_t current_frame;
  uint32_t current_image_index;
  bool framebuffer_resized;

  VkFormat swapchain_image_format;
  VkImage *swapchain_images;
  VkImageView *swapchain_image_views;

  VkImage depth_image;
  VmaAllocation depth_image_allocation;
  VkImageView depth_image_view;

  VkFramebuffer *swapchain_framebuffers;

  MtRenderPass render_pass;

  MtICmdBuffer cmd_buffers[FRAMES_IN_FLIGHT];
} MtWindow;

/*
 *
 * Event queue
 *
 */
enum { EVENT_QUEUE_CAPACITY = 65535 };

typedef struct EventQueue {
  MtEvent events[EVENT_QUEUE_CAPACITY];
  size_t head;
  size_t tail;
} EventQueue;

static EventQueue g_event_queue = {0};

/*
 * Event Callbacks
 */

static MtEvent *new_event();
static void window_pos_callback(GLFWwindow *window, int x, int y);
static void window_size_callback(GLFWwindow *window, int width, int height);
static void window_close_callback(GLFWwindow *window);
static void window_refresh_callback(GLFWwindow *window);
static void window_focus_callback(GLFWwindow *window, int focused);
static void window_iconify_callback(GLFWwindow *window, int iconified);
static void
framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void
mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void cursor_pos_callback(GLFWwindow *window, double x, double y);
static void cursor_enter_callback(GLFWwindow *window, int entered);
static void scroll_callback(GLFWwindow *window, double x, double y);
static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void char_callback(GLFWwindow *window, unsigned int codepoint);
static void monitor_callback(GLFWmonitor *monitor, int action);
static void joystick_callback(int jid, int action);
static void window_maximize_callback(GLFWwindow *window, int maximized);
static void
window_content_scale_callback(GLFWwindow *window, float xscale, float yscale);

/*
 * Window System
 */

static const char **get_instance_extensions(uint32_t *count) {
  return glfwGetRequiredInstanceExtensions(count);
}

static const int32_t get_physical_device_presentation_support(
    VkInstance instance, VkPhysicalDevice device, uint32_t queue_family) {
  return (int32_t)glfwGetPhysicalDevicePresentationSupport(
      instance, device, queue_family);
}

static MtWindowSystem g_glfw_window_system = (MtWindowSystem){
    .get_vulkan_instance_extensions = get_instance_extensions,
    .get_physical_device_presentation_support =
        get_physical_device_presentation_support,
};

/*
 * Window System VT
 */

static void poll_events(void) { glfwPollEvents(); }

static void glfw_destroy(void) { glfwTerminate(); }

static MtWindowSystemVT g_glfw_window_system_vt = (MtWindowSystemVT){
    .poll_events = poll_events,
    .destroy     = glfw_destroy,
};

/*
 * Setup functions
 */

typedef struct SwapchainSupport {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  uint32_t format_count;
  VkPresentModeKHR *present_modes;
  uint32_t present_mode_count;
} SwapchainSupport;

static void create_semaphores(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  VkSemaphoreCreateInfo semaphore_info = {0};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
    VK_CHECK(vkCreateSemaphore(
        dev->device,
        &semaphore_info,
        NULL,
        &window->image_available_semaphores[i]));
    VK_CHECK(vkCreateSemaphore(
        dev->device,
        &semaphore_info,
        NULL,
        &window->render_finished_semaphores[i]));
  }
}

static void create_fences(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  VkFenceCreateInfo fence_info = {0};
  fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
    VK_CHECK(vkCreateFence(
        dev->device, &fence_info, NULL, &window->in_flight_fences[i]));
  }
}

static SwapchainSupport query_swapchain_support(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  SwapchainSupport details = {0};

  // Capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      dev->physical_device, window->surface, &details.capabilities);

  // Formats
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      dev->physical_device, window->surface, &details.format_count, NULL);
  if (details.format_count != 0) {
    details.formats = mt_alloc(
        window->arena, sizeof(VkSurfaceFormatKHR) * details.format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        dev->physical_device,
        window->surface,
        &details.format_count,
        details.formats);
  }

  // Present modes
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      dev->physical_device, window->surface, &details.present_mode_count, NULL);
  if (details.present_mode_count != 0) {
    details.present_modes = mt_alloc(
        window->arena, sizeof(VkPresentModeKHR) * details.present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        dev->physical_device,
        window->surface,
        &details.present_mode_count,
        details.present_modes);
  }

  return details;
}

static VkSurfaceFormatKHR
choose_swapchain_surface_format(VkSurfaceFormatKHR *formats, uint32_t count) {
  if (count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    VkSurfaceFormatKHR fmt = {
      format : VK_FORMAT_B8G8R8A8_UNORM,
      colorSpace : formats[0].colorSpace,
    };

    return fmt;
  }

  for (uint32_t i = 0; i < count; i++) {
    VkSurfaceFormatKHR format = formats[i];
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }

  return formats[0];
}

static VkPresentModeKHR
choose_swapchain_present_mode(VkPresentModeKHR *present_modes, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    VkPresentModeKHR mode = present_modes[i];
    if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      return mode;
    }
  }

  for (uint32_t i = 0; i < count; i++) {
    VkPresentModeKHR mode = present_modes[i];
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }

  for (uint32_t i = 0; i < count; i++) {
    VkPresentModeKHR mode = present_modes[i];
    if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
      return mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swapchain_extent(
    MtWindow *window, VkSurfaceCapabilitiesKHR capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window->window, &width, &height);
    VkExtent2D actual_extent = {(uint32_t)width, (uint32_t)height};

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

static void create_swapchain(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  SwapchainSupport swapchain_support = query_swapchain_support(window);

  if (swapchain_support.format_count == 0 ||
      swapchain_support.present_mode_count == 0) {
    printf("Physical device does not support swapchain creation\n");
    exit(1);
  }

  VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(
      swapchain_support.formats, swapchain_support.format_count);
  VkPresentModeKHR present_mode = choose_swapchain_present_mode(
      swapchain_support.present_modes, swapchain_support.present_mode_count);
  VkExtent2D extent =
      choose_swapchain_extent(window, swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

  if (swapchain_support.capabilities.maxImageCount > 0 &&
      image_count > swapchain_support.capabilities.maxImageCount) {
    image_count = swapchain_support.capabilities.maxImageCount;
  }

  VkImageUsageFlags image_usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (!(swapchain_support.capabilities.supportedUsageFlags &
        VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
    printf("Physical device does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT "
           "in swapchains\n");
    exit(1);
  }

  VkSwapchainCreateInfoKHR create_info = {
      .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface          = window->surface,
      .minImageCount    = image_count,
      .imageFormat      = surface_format.format,
      .imageColorSpace  = surface_format.colorSpace,
      .imageExtent      = extent,
      .imageArrayLayers = 1,
      .imageUsage       = image_usage,
  };

  uint32_t queue_family_indices[2] = {dev->indices.graphics,
                                      dev->indices.present};

  if (dev->indices.graphics != dev->indices.present) {
    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices   = queue_family_indices;
  } else {
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = NULL;
  }

  create_info.preTransform   = swapchain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  create_info.presentMode = present_mode;
  create_info.clipped     = VK_TRUE;

  VkSwapchainKHR old_swapchain = window->swapchain;
  create_info.oldSwapchain     = old_swapchain;

  VK_CHECK(vkCreateSwapchainKHR(
      dev->device, &create_info, NULL, &window->swapchain));

  if (old_swapchain) {
    vkDestroySwapchainKHR(dev->device, old_swapchain, NULL);
  }

  vkGetSwapchainImagesKHR(dev->device, window->swapchain, &image_count, NULL);
  window->swapchain_images = mt_realloc(
      window->arena, window->swapchain_images, sizeof(VkImage) * image_count);
  vkGetSwapchainImagesKHR(
      dev->device, window->swapchain, &image_count, window->swapchain_images);

  window->swapchain_image_count  = image_count;
  window->swapchain_image_format = surface_format.format;
  window->render_pass.extent     = extent;

  mt_free(window->arena, swapchain_support.formats);
  mt_free(window->arena, swapchain_support.present_modes);
}

static void create_swapchain_image_views(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  window->swapchain_image_views = mt_realloc(
      window->arena,
      window->swapchain_image_views,
      sizeof(VkImageView) * window->swapchain_image_count);

  for (size_t i = 0; i < window->swapchain_image_count; i++) {
    VkImageViewCreateInfo create_info = {
        .sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image                       = window->swapchain_images[i],
        .viewType                    = VK_IMAGE_VIEW_TYPE_2D,
        .format                      = window->swapchain_image_format,
        .components.r                = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g                = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b                = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a                = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
    };

    VK_CHECK(vkCreateImageView(
        dev->device, &create_info, NULL, &window->swapchain_image_views[i]));
  }
}

static void create_depth_images(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  VkImageCreateInfo image_create_info = {
      .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format    = dev->preferred_depth_format,
      .extent =
          {
              .width  = window->render_pass.extent.width,
              .height = window->render_pass.extent.height,
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
      &window->depth_image,
      &window->depth_image_allocation,
      NULL));

  VkImageViewCreateInfo create_info = {
      .sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image        = window->depth_image,
      .viewType     = VK_IMAGE_VIEW_TYPE_2D,
      .format       = dev->preferred_depth_format,
      .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
      .subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
  };

  VK_CHECK(vkCreateImageView(
      dev->device, &create_info, NULL, &window->depth_image_view));
}

static void create_renderpass(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  VkAttachmentDescription color_attachment = {
      .format         = window->swapchain_image_format,
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
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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
      dev->device,
      &renderpass_create_info,
      NULL,
      &window->render_pass.renderpass));

  window->render_pass.hash = vulkan_hash_render_pass(&renderpass_create_info);
}

static void create_framebuffers(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  window->swapchain_framebuffers = mt_realloc(
      window->arena,
      window->swapchain_framebuffers,
      sizeof(VkFramebuffer) * window->swapchain_image_count);

  for (size_t i = 0; i < window->swapchain_image_count; i++) {
    VkImageView attachments[2] = {window->swapchain_image_views[i],
                                  window->depth_image_view};

    VkFramebufferCreateInfo create_info = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = window->render_pass.renderpass,
        .attachmentCount = MT_LENGTH(attachments),
        .pAttachments    = attachments,
        .width           = window->render_pass.extent.width,
        .height          = window->render_pass.extent.height,
        .layers          = 1,
    };

    VK_CHECK(vkCreateFramebuffer(
        dev->device, &create_info, NULL, &window->swapchain_framebuffers[i]));
  }
}

static void create_resizables(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  int width = 0, height = 0;
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window->window, &width, &height);
    glfwWaitEvents();
  }

  VK_CHECK(vkDeviceWaitIdle(dev->device));

  create_swapchain(window);
  create_swapchain_image_views(window);
  create_depth_images(window);
  create_renderpass(window);
  create_framebuffers(window);
}

static void destroy_resizables(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  VK_CHECK(vkDeviceWaitIdle(dev->device));

  for (uint32_t i = 0; i < window->swapchain_image_count; i++) {
    VkFramebuffer *framebuffer = &window->swapchain_framebuffers[i];
    if (framebuffer) {
      vkDestroyFramebuffer(dev->device, *framebuffer, NULL);
      *framebuffer = VK_NULL_HANDLE;
    }
  }

  if (window->render_pass.renderpass) {
    vkDestroyRenderPass(dev->device, window->render_pass.renderpass, NULL);
    window->render_pass.renderpass = VK_NULL_HANDLE;
  }

  for (uint32_t i = 0; i < window->swapchain_image_count; i++) {
    VkImageView *image_view = &window->swapchain_image_views[i];
    if (image_view) {
      vkDestroyImageView(dev->device, *image_view, NULL);
      *image_view = VK_NULL_HANDLE;
    }
  }

  if (window->depth_image) {
    vmaDestroyImage(
        dev->gpu_allocator,
        window->depth_image,
        window->depth_image_allocation);
    vkDestroyImageView(dev->device, window->depth_image_view, NULL);
  }

  if (window->swapchain) {
    vkDestroySwapchainKHR(dev->device, window->swapchain, NULL);
    window->swapchain = VK_NULL_HANDLE;
  }
}

static void allocate_cmd_buffers(MtWindow *window) {
  window->dev.vt->allocate_cmd_buffers(
      window->dev.inst,
      MT_QUEUE_GRAPHICS,
      FRAMES_IN_FLIGHT,
      window->cmd_buffers);
}

static void free_cmd_buffers(MtWindow *window) {
  window->dev.vt->free_cmd_buffers(
      window->dev.inst,
      MT_QUEUE_GRAPHICS,
      FRAMES_IN_FLIGHT,
      window->cmd_buffers);
}

/*
 * Window VT
 */

static bool should_close(MtWindow *window) {
  glfwWindowShouldClose(window->window);
}

static bool next_event(MtWindow *window, MtEvent *event) {
  memset(event, 0, sizeof(MtEvent));

  if (g_event_queue.head != g_event_queue.tail) {
    *event             = g_event_queue.events[g_event_queue.tail];
    g_event_queue.tail = (g_event_queue.tail + 1) % EVENT_QUEUE_CAPACITY;
  }

  /* switch (event->type) { */
  /* case MT_EVENT_FRAMEBUFFER_RESIZED: { */
  /*   update_size(window); */
  /*   break; */
  /* } */
  /* default: break; */
  /* } */

  return event->type != MT_EVENT_NONE;
}

static MtICmdBuffer begin_frame(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  vkWaitForFences(
      dev->device,
      1,
      &window->in_flight_fences[window->current_frame],
      VK_TRUE,
      UINT64_MAX);

  VkResult res = vkAcquireNextImageKHR(
      dev->device,
      window->swapchain,
      UINT64_MAX,
      window->image_available_semaphores[window->current_frame],
      VK_NULL_HANDLE,
      &window->current_image_index);

  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    destroy_resizables(window);
    create_resizables(window);
    return begin_frame(window);
  } else {
    VK_CHECK(res);
  }

  window->render_pass.current_framebuffer =
      window->swapchain_framebuffers[window->current_image_index];

  return window->cmd_buffers[window->current_frame];
}

static void end_frame(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  VkSubmitInfo submit_info = {0};
  submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[1] = {
      window->image_available_semaphores[window->current_frame]};
  VkSemaphore signal_semaphores[1] = {
      window->render_finished_semaphores[window->current_frame]};

  submit_info.waitSemaphoreCount = MT_LENGTH(wait_semaphores);
  submit_info.pWaitSemaphores    = wait_semaphores;

  VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submit_info.pWaitDstStageMask = &wait_stage;

  MtCmdBuffer *cmd_buffer_inst =
      window->cmd_buffers[window->current_frame].inst;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &cmd_buffer_inst->cmd_buffer;

  submit_info.signalSemaphoreCount = MT_LENGTH(signal_semaphores);
  submit_info.pSignalSemaphores    = signal_semaphores;

  vkResetFences(
      dev->device, 1, &window->in_flight_fences[window->current_frame]);
  VK_CHECK(vkQueueSubmit(
      dev->graphics_queue,
      1,
      &submit_info,
      window->in_flight_fences[window->current_frame]));

  VkPresentInfoKHR present_info = {
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = MT_LENGTH(signal_semaphores),
      .pWaitSemaphores    = signal_semaphores,
      .swapchainCount     = 1,
      .pSwapchains        = &window->swapchain,
      .pImageIndices      = &window->current_image_index,
  };

  VkResult res = vkQueuePresentKHR(dev->present_queue, &present_info);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR ||
      window->framebuffer_resized) {
    window->framebuffer_resized = false;
    destroy_resizables(window);
    create_resizables(window);
  } else {
    VK_CHECK(res);
  }

  window->current_frame = (window->current_frame + 1) % FRAMES_IN_FLIGHT;
}

static void window_destroy(MtWindow *window) {
  MtDevice *dev = window->dev.inst;

  VK_CHECK(vkDeviceWaitIdle(dev->device));

  destroy_resizables(window);

  for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(
        dev->device, window->image_available_semaphores[i], NULL);
    vkDestroySemaphore(
        dev->device, window->render_finished_semaphores[i], NULL);
    vkDestroyFence(dev->device, window->in_flight_fences[i], NULL);
  }

  vkDestroySurfaceKHR(dev->instance, window->surface, NULL);

  glfwDestroyWindow(window->window);
}

static MtRenderPass *get_render_pass(MtWindow *window) {
  return &window->render_pass;
}

static MtWindowVT g_glfw_window_vt = (MtWindowVT){
    .should_close = should_close,
    .next_event   = next_event,

    .begin_frame     = begin_frame,
    .end_frame       = end_frame,
    .get_render_pass = get_render_pass,

    .destroy = window_destroy,
};

/*
 * Public functions
 */

void mt_glfw_vulkan_init(MtIWindowSystem *system) {
  VK_CHECK(volkInitialize());

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  if (!glfwVulkanSupported()) {
    printf("Vulkan is not supported\n");
    exit(1);
  }

  system->inst = (MtWindowSystem *)&g_glfw_window_system;
  system->vt   = &g_glfw_window_system_vt;
}

void mt_glfw_vulkan_window_init(
    MtIWindow *interface,
    MtIDevice *idev,
    uint32_t width,
    uint32_t height,
    const char *title,
    MtArena *arena) {
  MtWindow *window = mt_calloc(arena, sizeof(MtWindow));
  window->window = glfwCreateWindow((int)width, (int)height, title, NULL, NULL);
  window->dev    = *idev;
  window->arena  = arena;
  MtDevice *dev  = window->dev.inst;

  interface->vt   = &g_glfw_window_vt;
  interface->inst = (MtWindow *)window;

  glfwSetWindowUserPointer(window->window, interface);

  // Set callbacks
  glfwSetMonitorCallback(monitor_callback);
  glfwSetJoystickCallback(joystick_callback);

  glfwSetWindowPosCallback(window->window, window_pos_callback);
  glfwSetWindowSizeCallback(window->window, window_size_callback);
  glfwSetWindowCloseCallback(window->window, window_close_callback);
  glfwSetWindowRefreshCallback(window->window, window_refresh_callback);
  glfwSetWindowFocusCallback(window->window, window_focus_callback);
  glfwSetWindowIconifyCallback(window->window, window_iconify_callback);
  glfwSetFramebufferSizeCallback(window->window, framebuffer_size_callback);
  glfwSetMouseButtonCallback(window->window, mouse_button_callback);
  glfwSetCursorPosCallback(window->window, cursor_pos_callback);
  glfwSetCursorEnterCallback(window->window, cursor_enter_callback);
  glfwSetScrollCallback(window->window, scroll_callback);
  glfwSetKeyCallback(window->window, key_callback);
  glfwSetCharCallback(window->window, char_callback);
  glfwSetWindowMaximizeCallback(window->window, window_maximize_callback);
  glfwSetWindowContentScaleCallback(
      window->window, window_content_scale_callback);

  VK_CHECK(glfwCreateWindowSurface(
      dev->instance, window->window, NULL, &window->surface));

  VkBool32 supported;
  vkGetPhysicalDeviceSurfaceSupportKHR(
      dev->physical_device, dev->indices.present, window->surface, &supported);
  if (!supported) {
    printf("Physical device does not support surface\n");
    exit(1);
  }

  create_semaphores(window);
  create_fences(window);

  create_resizables(window);

  allocate_cmd_buffers(window);
}

static MtEvent *new_event() {
  MtEvent *event     = g_event_queue.events + g_event_queue.head;
  g_event_queue.head = (g_event_queue.head + 1) % EVENT_QUEUE_CAPACITY;
  assert(g_event_queue.head != g_event_queue.tail);
  memset(event, 0, sizeof(MtEvent));
  return event;
}

static void window_pos_callback(GLFWwindow *window, int x, int y) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->type    = MT_EVENT_WINDOW_MOVED;
  event->window  = win;
  event->pos.x   = x;
  event->pos.y   = y;
}

static void window_size_callback(GLFWwindow *window, int width, int height) {
  MtIWindow *win     = glfwGetWindowUserPointer(window);
  MtEvent *event     = new_event();
  event->type        = MT_EVENT_WINDOW_RESIZED;
  event->window      = win;
  event->size.width  = width;
  event->size.height = height;
}

static void window_close_callback(GLFWwindow *window) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->type    = MT_EVENT_WINDOW_CLOSED;
  event->window  = win;
}

static void window_refresh_callback(GLFWwindow *window) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->type    = MT_EVENT_WINDOW_REFRESH;
  event->window  = win;
}

static void window_focus_callback(GLFWwindow *window, int focused) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->window  = win;

  if (focused)
    event->type = MT_EVENT_WINDOW_FOCUSED;
  else
    event->type = MT_EVENT_WINDOW_DEFOCUSED;
}

static void window_iconify_callback(GLFWwindow *window, int iconified) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->window  = win;

  if (iconified)
    event->type = MT_EVENT_WINDOW_ICONIFIED;
  else
    event->type = MT_EVENT_WINDOW_UNICONIFIED;
}

static void
framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  MtIWindow *win     = glfwGetWindowUserPointer(window);
  MtEvent *event     = new_event();
  event->type        = MT_EVENT_FRAMEBUFFER_RESIZED;
  event->window      = win;
  event->size.width  = width;
  event->size.height = height;
}

static void
mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  MtIWindow *win      = glfwGetWindowUserPointer(window);
  MtEvent *event      = new_event();
  event->window       = win;
  event->mouse.button = button;
  event->mouse.mods   = mods;

  if (action == GLFW_PRESS)
    event->type = MT_EVENT_BUTTON_PRESSED;
  else if (action == GLFW_RELEASE)
    event->type = MT_EVENT_BUTTON_RELEASED;
}

static void cursor_pos_callback(GLFWwindow *window, double x, double y) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->type    = MT_EVENT_CURSOR_MOVED;
  event->window  = win;
  event->pos.x   = (int)x;
  event->pos.y   = (int)y;
}

static void cursor_enter_callback(GLFWwindow *window, int entered) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->window  = win;

  if (entered)
    event->type = MT_EVENT_CURSOR_ENTERED;
  else
    event->type = MT_EVENT_CURSOR_LEFT;
}

static void scroll_callback(GLFWwindow *window, double x, double y) {
  MtIWindow *win  = glfwGetWindowUserPointer(window);
  MtEvent *event  = new_event();
  event->type     = MT_EVENT_SCROLLED;
  event->window   = win;
  event->scroll.x = x;
  event->scroll.y = y;
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  MtIWindow *win           = glfwGetWindowUserPointer(window);
  MtEvent *event           = new_event();
  event->window            = win;
  event->keyboard.key      = key;
  event->keyboard.scancode = scancode;
  event->keyboard.mods     = mods;

  if (action == GLFW_PRESS)
    event->type = MT_EVENT_KEY_PRESSED;
  else if (action == GLFW_RELEASE)
    event->type = MT_EVENT_KEY_RELEASED;
  else if (action == GLFW_REPEAT)
    event->type = MT_EVENT_KEY_REPEATED;
}

static void char_callback(GLFWwindow *window, unsigned int codepoint) {
  MtIWindow *win   = glfwGetWindowUserPointer(window);
  MtEvent *event   = new_event();
  event->type      = MT_EVENT_CODEPOINT_INPUT;
  event->window    = win;
  event->codepoint = codepoint;
}

static void monitor_callback(GLFWmonitor *monitor, int action) {
  MtEvent *event = new_event();
  event->monitor = monitor;

  if (action == GLFW_CONNECTED)
    event->type = MT_EVENT_MONITOR_CONNECTED;
  else if (action == GLFW_DISCONNECTED)
    event->type = MT_EVENT_MONITOR_DISCONNECTED;
}

static void joystick_callback(int jid, int action) {
  MtEvent *event  = new_event();
  event->joystick = jid;

  if (action == GLFW_CONNECTED)
    event->type = MT_EVENT_JOYSTICK_CONNECTED;
  else if (action == GLFW_DISCONNECTED)
    event->type = MT_EVENT_JOYSTICK_DISCONNECTED;
}

static void window_maximize_callback(GLFWwindow *window, int maximized) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->window  = win;

  if (maximized)
    event->type = MT_EVENT_WINDOW_MAXIMIZED;
  else
    event->type = MT_EVENT_WINDOW_UNMAXIMIZED;
}

static void
window_content_scale_callback(GLFWwindow *window, float xscale, float yscale) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = new_event();
  event->window  = win;
  event->type    = MT_EVENT_WINDOW_SCALE_CHANGED;
  event->scale.x = xscale;
  event->scale.y = yscale;
}
