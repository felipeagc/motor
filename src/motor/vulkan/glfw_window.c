#include "../../../include/motor/vulkan/glfw_window.h"

#include "internal.h"
#include <GLFW/glfw3.h>
#include <string.h>
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/window.h"

/*
 *
 * Event queue
 *
 */
#define _MT_EVENT_QUEUE_CAPACITY 65535

typedef struct _MtEventQueue {
  MtEvent events[_MT_EVENT_QUEUE_CAPACITY];
  size_t head;
  size_t tail;
} _MtEventQueue;

static _MtEventQueue _g_event_queue = {0};

/*
 * Event Callbacks
 */

static MtEvent *_new_event();
static void _window_pos_callback(GLFWwindow *window, int x, int y);
static void _window_size_callback(GLFWwindow *window, int width, int height);
static void _window_close_callback(GLFWwindow *window);
static void _window_refresh_callback(GLFWwindow *window);
static void _window_focus_callback(GLFWwindow *window, int focused);
static void _window_iconify_callback(GLFWwindow *window, int iconified);
static void
_framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void
_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void _cursor_pos_callback(GLFWwindow *window, double x, double y);
static void _cursor_enter_callback(GLFWwindow *window, int entered);
static void _scroll_callback(GLFWwindow *window, double x, double y);
static void
_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void _char_callback(GLFWwindow *window, unsigned int codepoint);
static void _monitor_callback(GLFWmonitor *monitor, int action);
static void _joystick_callback(int jid, int action);
static void _window_maximize_callback(GLFWwindow *window, int maximized);
static void
_window_content_scale_callback(GLFWwindow *window, float xscale, float yscale);

/*
 * Window System
 */

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

/*
 * Window
 */

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

static bool _next_event(_MtGlfwWindow *window, MtEvent *event) {
  memset(event, 0, sizeof(MtEvent));

  if (_g_event_queue.head != _g_event_queue.tail) {
    *event              = _g_event_queue.events[_g_event_queue.tail];
    _g_event_queue.tail = (_g_event_queue.tail + 1) % _MT_EVENT_QUEUE_CAPACITY;
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

static bool _should_close(_MtGlfwWindow *window) {
  glfwWindowShouldClose(window->window);
}

static void _window_destroy(_MtGlfwWindow *window) {
  glfwDestroyWindow(window->window);
}

static MtWindowVT _MT_GLFW_WINDOW_VT = (MtWindowVT){
    .should_close = (void *)_should_close,
    .next_event   = (void *)_next_event,
    .destroy      = (void *)_window_destroy,
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

  glfwSetWindowUserPointer(window->window, interface);

  // Set callbacks
  glfwSetMonitorCallback(_monitor_callback);
  glfwSetJoystickCallback(_joystick_callback);

  glfwSetWindowPosCallback(window->window, _window_pos_callback);
  glfwSetWindowSizeCallback(window->window, _window_size_callback);
  glfwSetWindowCloseCallback(window->window, _window_close_callback);
  glfwSetWindowRefreshCallback(window->window, _window_refresh_callback);
  glfwSetWindowFocusCallback(window->window, _window_focus_callback);
  glfwSetWindowIconifyCallback(window->window, _window_iconify_callback);
  glfwSetFramebufferSizeCallback(window->window, _framebuffer_size_callback);
  glfwSetMouseButtonCallback(window->window, _mouse_button_callback);
  glfwSetCursorPosCallback(window->window, _cursor_pos_callback);
  glfwSetCursorEnterCallback(window->window, _cursor_enter_callback);
  glfwSetScrollCallback(window->window, _scroll_callback);
  glfwSetKeyCallback(window->window, _key_callback);
  glfwSetCharCallback(window->window, _char_callback);
  glfwSetWindowMaximizeCallback(window->window, _window_maximize_callback);
  glfwSetWindowContentScaleCallback(
      window->window, _window_content_scale_callback);
}

static MtEvent *_new_event() {
  MtEvent *event      = _g_event_queue.events + _g_event_queue.head;
  _g_event_queue.head = (_g_event_queue.head + 1) % _MT_EVENT_QUEUE_CAPACITY;
  assert(_g_event_queue.head != _g_event_queue.tail);
  memset(event, 0, sizeof(MtEvent));
  return event;
}

static void _window_pos_callback(GLFWwindow *window, int x, int y) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->type    = MT_EVENT_WINDOW_MOVED;
  event->window  = win;
  event->pos.x   = x;
  event->pos.y   = y;
}

static void _window_size_callback(GLFWwindow *window, int width, int height) {
  MtIWindow *win     = glfwGetWindowUserPointer(window);
  MtEvent *event     = _new_event();
  event->type        = MT_EVENT_WINDOW_RESIZED;
  event->window      = win;
  event->size.width  = width;
  event->size.height = height;
}

static void _window_close_callback(GLFWwindow *window) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->type    = MT_EVENT_WINDOW_CLOSED;
  event->window  = win;
}

static void _window_refresh_callback(GLFWwindow *window) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->type    = MT_EVENT_WINDOW_REFRESH;
  event->window  = win;
}

static void _window_focus_callback(GLFWwindow *window, int focused) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->window  = win;

  if (focused)
    event->type = MT_EVENT_WINDOW_FOCUSED;
  else
    event->type = MT_EVENT_WINDOW_DEFOCUSED;
}

static void _window_iconify_callback(GLFWwindow *window, int iconified) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->window  = win;

  if (iconified)
    event->type = MT_EVENT_WINDOW_ICONIFIED;
  else
    event->type = MT_EVENT_WINDOW_UNICONIFIED;
}

static void
_framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  MtIWindow *win     = glfwGetWindowUserPointer(window);
  MtEvent *event     = _new_event();
  event->type        = MT_EVENT_FRAMEBUFFER_RESIZED;
  event->window      = win;
  event->size.width  = width;
  event->size.height = height;
}

static void
_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  MtIWindow *win      = glfwGetWindowUserPointer(window);
  MtEvent *event      = _new_event();
  event->window       = win;
  event->mouse.button = button;
  event->mouse.mods   = mods;

  if (action == GLFW_PRESS)
    event->type = MT_EVENT_BUTTON_PRESSED;
  else if (action == GLFW_RELEASE)
    event->type = MT_EVENT_BUTTON_RELEASED;
}

static void _cursor_pos_callback(GLFWwindow *window, double x, double y) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->type    = MT_EVENT_CURSOR_MOVED;
  event->window  = win;
  event->pos.x   = (int)x;
  event->pos.y   = (int)y;
}

static void _cursor_enter_callback(GLFWwindow *window, int entered) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->window  = win;

  if (entered)
    event->type = MT_EVENT_CURSOR_ENTERED;
  else
    event->type = MT_EVENT_CURSOR_LEFT;
}

static void _scroll_callback(GLFWwindow *window, double x, double y) {
  MtIWindow *win  = glfwGetWindowUserPointer(window);
  MtEvent *event  = _new_event();
  event->type     = MT_EVENT_SCROLLED;
  event->window   = win;
  event->scroll.x = x;
  event->scroll.y = y;
}

static void
_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  MtIWindow *win           = glfwGetWindowUserPointer(window);
  MtEvent *event           = _new_event();
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

static void _char_callback(GLFWwindow *window, unsigned int codepoint) {
  MtIWindow *win   = glfwGetWindowUserPointer(window);
  MtEvent *event   = _new_event();
  event->type      = MT_EVENT_CODEPOINT_INPUT;
  event->window    = win;
  event->codepoint = codepoint;
}

static void _monitor_callback(GLFWmonitor *monitor, int action) {
  MtEvent *event = _new_event();
  event->monitor = monitor;

  if (action == GLFW_CONNECTED)
    event->type = MT_EVENT_MONITOR_CONNECTED;
  else if (action == GLFW_DISCONNECTED)
    event->type = MT_EVENT_MONITOR_DISCONNECTED;
}

static void _joystick_callback(int jid, int action) {
  MtEvent *event  = _new_event();
  event->joystick = jid;

  if (action == GLFW_CONNECTED)
    event->type = MT_EVENT_JOYSTICK_CONNECTED;
  else if (action == GLFW_DISCONNECTED)
    event->type = MT_EVENT_JOYSTICK_DISCONNECTED;
}

static void _window_maximize_callback(GLFWwindow *window, int maximized) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->window  = win;

  if (maximized)
    event->type = MT_EVENT_WINDOW_MAXIMIZED;
  else
    event->type = MT_EVENT_WINDOW_UNMAXIMIZED;
}

static void
_window_content_scale_callback(GLFWwindow *window, float xscale, float yscale) {
  MtIWindow *win = glfwGetWindowUserPointer(window);
  MtEvent *event = _new_event();
  event->window  = win;
  event->type    = MT_EVENT_WINDOW_SCALE_CHANGED;
  event->scale.x = xscale;
  event->scale.y = yscale;
}
