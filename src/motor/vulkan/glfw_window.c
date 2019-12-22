#include "../../../include/motor/vulkan/glfw_window.h"

#include "internal.h"
#include <GLFW/glfw3.h>
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/window.h"

static const char **_get_instance_extensions(uint32_t *count) {
  return glfwGetRequiredInstanceExtensions(count);
}

static const int32_t _get_physical_device_presentation_support(
    VkInstance instance, VkPhysicalDevice device, uint32_t queue_family) {
  return (int32_t)glfwGetPhysicalDevicePresentationSupport(
      instance, device, queue_family);
}

static MtVulkanWindowSystem _MT_GLFW_WINDOW_SYSTEM = (MtVulkanWindowSystem){
    .get_vulkan_instance_extensions = _get_instance_extensions,
    .get_physical_device_presentation_support =
        _get_physical_device_presentation_support,
};

typedef struct _MtGlfwWindow {
  GLFWwindow *window;
} _MtGlfwWindow;

/*
 * Window System VT
 */

static void _poll_events(void) { glfwPollEvents(); }

static void _glfw_destroy(void) { glfwTerminate(); }

static MtWindowSystemVT _MT_GLFW_WINDOW_SYSTEM_VT = (MtWindowSystemVT){
    .poll_events = (void *)_poll_events,
    .destroy     = (void *)_glfw_destroy,
};

/*
 * Window VT
 */

static void _window_destroy(_MtGlfwWindow *window) {
  glfwDestroyWindow(window->window);
}

static bool _should_close(_MtGlfwWindow *window) {
  glfwWindowShouldClose(window->window);
}

static MtWindowVT _MT_GLFW_WINDOW_VT = (MtWindowVT){
    .destroy      = (void *)_window_destroy,
    .should_close = (void *)_should_close,
};

/*
 * Public functions
 */

void mt_glfw_vulkan_init(MtIWindowSystem *system) {
  VK_CHECK(volkInitialize());

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  system->inst = (MtWindowSystem *)&_MT_GLFW_WINDOW_SYSTEM;
  system->vt   = &_MT_GLFW_WINDOW_SYSTEM_VT;
}

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
