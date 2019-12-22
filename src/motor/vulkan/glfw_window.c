#include "../../../include/motor/vulkan/glfw_window.h"

#include "internal.h"
#include <GLFW/glfw3.h>
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/window.h"

static const char **_mt_glfw_get_instance_extensions(uint32_t *count) {
  return glfwGetRequiredInstanceExtensions(count);
}

static const int32_t _mt_glfw_get_physical_device_presentation_support(
    VkInstance instance, VkPhysicalDevice device, uint32_t queue_family) {
  return (int32_t)glfwGetPhysicalDevicePresentationSupport(
      instance, device, queue_family);
}

static MtVulkanWindowSystem _MT_GLFW_WINDOW_SYSTEM = (MtVulkanWindowSystem){
    .get_vulkan_instance_extensions = _mt_glfw_get_instance_extensions,
    .get_physical_device_presentation_support =
        _mt_glfw_get_physical_device_presentation_support,
};

typedef struct _MtGlfwWindow {
  GLFWwindow *window;
} _MtGlfwWindow;

static void _mt_glfw_vulkan_window_destroy(_MtGlfwWindow *window);

static MtWindowVT _MT_GLFW_WINDOW_VT = (MtWindowVT){
    .destroy = (void *)_mt_glfw_vulkan_window_destroy,
};

MtVulkanWindowSystem *mt_glfw_vulkan_init() {
  VK_CHECK(volkInitialize());

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  return (MtVulkanWindowSystem *)&_MT_GLFW_WINDOW_SYSTEM;
}

void mt_glfw_vulkan_destroy() { glfwTerminate(); }

void mt_glfw_vulkan_window_init(
    MtIWindow *interface,
    uint32_t width,
    uint32_t height,
    const char *title,
    MtArena *arena) {
  _MtGlfwWindow *window = mt_alloc(arena, sizeof(_MtGlfwWindow));
  window->window = glfwCreateWindow((int)width, (int)height, title, NULL, NULL);

  interface->vt   = &_MT_GLFW_WINDOW_VT;
  interface->inst = (MtWindow *)window;
}

static void _mt_glfw_vulkan_window_destroy(_MtGlfwWindow *window) {
  glfwDestroyWindow(window->window);
}
