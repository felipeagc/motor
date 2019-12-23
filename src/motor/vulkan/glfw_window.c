#include "../../../include/motor/vulkan/glfw_window.h"

#include "internal.h"
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/window.h"

enum { FRAMES_IN_FLIGHT = 2 };

/*
 * Window
 */

typedef struct GlfwVulkanWindow {
  MtIDevice dev;
  GLFWwindow *window;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  VkSemaphore image_available_semaphores[FRAMES_IN_FLIGHT];
  VkSemaphore render_finished_semaphores[FRAMES_IN_FLIGHT];
  VkFence in_flight_fences[FRAMES_IN_FLIGHT];
} GlfwVulkanWindow;

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

static VulkanWindowSystem g_glfw_window_system = (VulkanWindowSystem){
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
    .poll_events = (void *)poll_events,
    .destroy     = (void *)glfw_destroy,
};

/*
 * Window VT
 */

static bool next_event(GlfwVulkanWindow *window, MtEvent *event) {
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

static bool should_close(GlfwVulkanWindow *window) {
  glfwWindowShouldClose(window->window);
}

static void window_destroy(GlfwVulkanWindow *window) {
  VulkanDevice *dev = (VulkanDevice *)window->dev.inst;

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

static MtWindowVT g_glfw_window_vt = (MtWindowVT){
    .should_close = (void *)should_close,
    .next_event   = (void *)next_event,
    .destroy      = (void *)window_destroy,
};

/*
 * Setup functions
 */

static void create_semaphores(GlfwVulkanWindow *window) {
  VulkanDevice *dev = (VulkanDevice *)window->dev.inst;

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

static void create_fences(GlfwVulkanWindow *window) {
  VulkanDevice *dev = (VulkanDevice *)window->dev.inst;

  VkFenceCreateInfo fence_info = {0};
  fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
    VK_CHECK(vkCreateFence(
        dev->device, &fence_info, NULL, &window->in_flight_fences[i]));
  }
}

/*
 * Public functions
 */

void mt_glfw_vulkan_init(MtIWindowSystem *system) {
  VK_CHECK(volkInitialize());

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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
  GlfwVulkanWindow *window = mt_alloc(arena, sizeof(GlfwVulkanWindow));
  window->window = glfwCreateWindow((int)width, (int)height, title, NULL, NULL);
  window->dev    = *idev;
  VulkanDevice *dev = (VulkanDevice *)window->dev.inst;

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
