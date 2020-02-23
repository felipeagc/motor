#include <motor/graphics/vulkan/glfw_window.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "internal.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <motor/base/api_types.h>
#include <motor/base/allocator.h>
#include <motor/base/log.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>

/*
 * Window
 */

typedef struct MtWindow
{
    MtAllocator *alloc;
    GLFWwindow *window;
    GLFWcursor *mouse_cursors[MT_CURSOR_TYPE_MAX];
} MtWindow;

/*
 *
 * Event queue
 *
 */
enum { EVENT_QUEUE_CAPACITY = 65535 };

typedef struct EventQueue
{
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
static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void cursor_pos_callback(GLFWwindow *window, double x, double y);
static void cursor_enter_callback(GLFWwindow *window, int entered);
static void scroll_callback(GLFWwindow *window, double x, double y);
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void char_callback(GLFWwindow *window, unsigned int codepoint);
static void monitor_callback(GLFWmonitor *monitor, int action);
static void joystick_callback(int jid, int action);
static void window_maximize_callback(GLFWwindow *window, int maximized);
static void window_content_scale_callback(GLFWwindow *window, float xscale, float yscale);

/*
 * Window VT
 */

static bool should_close(MtWindow *window)
{
    return glfwWindowShouldClose(window->window);
}

static bool next_event(MtWindow *window, MtEvent *event)
{
    memset(event, 0, sizeof(MtEvent));

    if (g_event_queue.head != g_event_queue.tail)
    {
        *event = g_event_queue.events[g_event_queue.tail];
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

static void wait_events(MtWindow *_window)
{
    glfwWaitEvents();
}

static MtWindow *
create_window(uint32_t width, uint32_t height, const char *title, MtAllocator *alloc)
{
    MtWindow *window = mt_alloc(alloc, sizeof(MtWindow));
    memset(window, 0, sizeof(*window));

    window->window = glfwCreateWindow((int)width, (int)height, title, NULL, NULL);
    window->alloc = alloc;

    // Center window
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (!mode)
        return NULL;

    int monitor_x, monitor_y;
    glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

    int window_width, window_height;
    glfwGetWindowSize(window->window, &window_width, &window_height);

    glfwSetWindowPos(
        window->window,
        monitor_x + (mode->width - window_width) / 2,
        monitor_y + (mode->height - window_height) / 2);

    window->mouse_cursors[MT_CURSOR_TYPE_ARROW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    window->mouse_cursors[MT_CURSOR_TYPE_IBEAM] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    window->mouse_cursors[MT_CURSOR_TYPE_CROSSHAIR] =
        glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    window->mouse_cursors[MT_CURSOR_TYPE_HAND] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    window->mouse_cursors[MT_CURSOR_TYPE_HRESIZE] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    window->mouse_cursors[MT_CURSOR_TYPE_VRESIZE] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);

    glfwSetWindowUserPointer(window->window, window);

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
    glfwSetWindowContentScaleCallback(window->window, window_content_scale_callback);

    return window;
}

static void destroy_window(MtWindow *window)
{
    for (uint32_t i = 0; i < MT_LENGTH(window->mouse_cursors); i++)
    {
        glfwDestroyCursor(window->mouse_cursors[i]);
    }

    glfwDestroyWindow(window->window);
}

static void get_size(MtWindow *window, uint32_t *width, uint32_t *height)
{
    int w = 0, h = 0;
    glfwGetFramebufferSize(window->window, &w, &h);
    *width = (uint32_t)w;
    *height = (uint32_t)h;
}

static void get_cursor_pos(MtWindow *w, int32_t *x, int32_t *y)
{
    double dx, dy;
    glfwGetCursorPos(w->window, &dx, &dy);
    *x = (int32_t)dx;
    *y = (int32_t)dy;
}

static void set_cursor_pos(MtWindow *w, int32_t x, int32_t y)
{
    glfwSetCursorPos(w->window, (double)x, (double)y);
}

static void get_cursor_pos_normalized(MtWindow *w, float *nx, float *ny)
{
    int32_t ix, iy;
    get_cursor_pos(w, &ix, &iy);

    uint32_t iw, ih;
    get_size(w, &iw, &ih);

    float fx = (float)ix, fy = (float)iy;
    float fw = (float)iw, fh = (float)ih;

    *nx = (fx / (fw * 0.5f)) - 1.0f;
    *ny = (fy / (fh * 0.5f)) - 1.0f;
}

static MtCursorMode get_cursor_mode(MtWindow *w)
{
    int mode = glfwGetInputMode(w->window, GLFW_CURSOR);
    switch (mode)
    {
        case GLFW_CURSOR_NORMAL: return MT_CURSOR_MODE_NORMAL;
        case GLFW_CURSOR_HIDDEN: return MT_CURSOR_MODE_HIDDEN;
        case GLFW_CURSOR_DISABLED: return MT_CURSOR_MODE_DISABLED;
    }
    return MT_CURSOR_MODE_NORMAL;
}

static void set_cursor_mode(MtWindow *w, MtCursorMode mode)
{
    int cursor_mode = MT_CURSOR_MODE_NORMAL;
    switch (mode)
    {
        case MT_CURSOR_MODE_NORMAL: cursor_mode = GLFW_CURSOR_NORMAL; break;
        case MT_CURSOR_MODE_HIDDEN: cursor_mode = GLFW_CURSOR_HIDDEN; break;
        case MT_CURSOR_MODE_DISABLED: cursor_mode = GLFW_CURSOR_DISABLED; break;
    }
    glfwSetInputMode(w->window, GLFW_CURSOR, cursor_mode);
}

static void set_cursor_type(MtWindow *w, MtCursorType type)
{
    glfwSetCursor(w->window, w->mouse_cursors[type]);
}

static MtInputState get_key(MtWindow *w, MtKey key_code)
{
    switch (glfwGetKey(w->window, (int32_t)key_code))
    {
        case GLFW_RELEASE: return MT_INPUT_STATE_RELEASE;
        case GLFW_PRESS: return MT_INPUT_STATE_PRESS;
        case GLFW_REPEAT: return MT_INPUT_STATE_REPEAT;
    }
    return MT_INPUT_STATE_RELEASE;
}

static MtInputState get_mouse_button(MtWindow *w, MtMouseButton button)
{
    switch (glfwGetMouseButton(w->window, button))
    {
        case GLFW_RELEASE: return MT_INPUT_STATE_RELEASE;
        case GLFW_PRESS: return MT_INPUT_STATE_PRESS;
        case GLFW_REPEAT: return MT_INPUT_STATE_REPEAT;
    }
    return MT_INPUT_STATE_RELEASE;
}

#if defined(_WIN32)
HWND get_win32_window(MtWindow *window)
{
    return glfwGetWin32Window(window->window);
}
#elif defined(__APPLE__)
#error Apple not supported
#else
Window get_x11_window(MtWindow *window)
{
    return glfwGetX11Window(window->window);
}

Display *get_x11_display(MtWindow *window)
{
    return glfwGetX11Display();
}
#endif

/*
 * Window System functions
 */

static void poll_events(void)
{
    glfwPollEvents();
}

static void destroy_window_system(void)
{
    glfwTerminate();
}

static MtWindowSystem g_glfw_window_system = {
    .poll_events = poll_events,
    .destroy_window_system = destroy_window_system,

    .should_close = should_close,
    .next_event = next_event,
    .wait_events = wait_events,

    .get_size = get_size,

    .get_cursor_pos = get_cursor_pos,
    .set_cursor_pos = set_cursor_pos,

    .get_cursor_pos_normalized = get_cursor_pos_normalized,

    .get_cursor_mode = get_cursor_mode,
    .set_cursor_mode = set_cursor_mode,

    .set_cursor_type = set_cursor_type,

    .get_key = get_key,
    .get_mouse_button = get_mouse_button,

    .create = create_window,
    .destroy = destroy_window,

#if defined(_WIN32)
    .get_win32_window = get_win32_window,
#elif defined(__APPLE__)
#error Apple not supported
#else
    .get_x11_window = get_x11_window,
    .get_x11_display = get_x11_display,
#endif
};

/*
 * Public functions
 */

void mt_glfw_vulkan_window_system_init(void)
{
    VK_CHECK(volkInitialize());

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!glfwVulkanSupported())
    {
        mt_log_fatal("Vulkan is not supported\n");
        exit(1);
    }

    mt_window = g_glfw_window_system;
}

static MtEvent *new_event()
{
    MtEvent *event = g_event_queue.events + g_event_queue.head;
    g_event_queue.head = (g_event_queue.head + 1) % EVENT_QUEUE_CAPACITY;
    assert(g_event_queue.head != g_event_queue.tail);
    memset(event, 0, sizeof(MtEvent));
    return event;
}

static void window_pos_callback(GLFWwindow *window, int x, int y)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_WINDOW_MOVED;
    event->window = win;
    event->pos.x = x;
    event->pos.y = y;
}

static void window_size_callback(GLFWwindow *window, int width, int height)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_WINDOW_RESIZED;
    event->window = win;
    event->size.width = width;
    event->size.height = height;
}

static void window_close_callback(GLFWwindow *window)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_WINDOW_CLOSED;
    event->window = win;
}

static void window_refresh_callback(GLFWwindow *window)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_WINDOW_REFRESH;
    event->window = win;
}

static void window_focus_callback(GLFWwindow *window, int focused)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->window = win;

    if (focused)
        event->type = MT_EVENT_WINDOW_FOCUSED;
    else
        event->type = MT_EVENT_WINDOW_DEFOCUSED;
}

static void window_iconify_callback(GLFWwindow *window, int iconified)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->window = win;

    if (iconified)
        event->type = MT_EVENT_WINDOW_ICONIFIED;
    else
        event->type = MT_EVENT_WINDOW_UNICONIFIED;
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_FRAMEBUFFER_RESIZED;
    event->window = win;
    event->size.width = width;
    event->size.height = height;
}

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->window = win;
    event->mouse.button = button;
    event->mouse.mods = mods;

    if (action == GLFW_PRESS)
        event->type = MT_EVENT_BUTTON_PRESSED;
    else if (action == GLFW_RELEASE)
        event->type = MT_EVENT_BUTTON_RELEASED;
}

static void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_CURSOR_MOVED;
    event->window = win;
    event->pos.x = (int)x;
    event->pos.y = (int)y;
}

static void cursor_enter_callback(GLFWwindow *window, int entered)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->window = win;

    if (entered)
        event->type = MT_EVENT_CURSOR_ENTERED;
    else
        event->type = MT_EVENT_CURSOR_LEFT;
}

static void scroll_callback(GLFWwindow *window, double x, double y)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_SCROLLED;
    event->window = win;
    event->scroll.x = x;
    event->scroll.y = y;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->window = win;
    event->keyboard.key = key;
    event->keyboard.scancode = scancode;
    event->keyboard.mods = mods;

    if (action == GLFW_PRESS)
        event->type = MT_EVENT_KEY_PRESSED;
    else if (action == GLFW_RELEASE)
        event->type = MT_EVENT_KEY_RELEASED;
    else if (action == GLFW_REPEAT)
        event->type = MT_EVENT_KEY_REPEATED;
}

static void char_callback(GLFWwindow *window, unsigned int codepoint)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->type = MT_EVENT_CODEPOINT_INPUT;
    event->window = win;
    event->codepoint = codepoint;
}

static void monitor_callback(GLFWmonitor *monitor, int action)
{
    MtEvent *event = new_event();
    event->monitor = monitor;

    if (action == GLFW_CONNECTED)
        event->type = MT_EVENT_MONITOR_CONNECTED;
    else if (action == GLFW_DISCONNECTED)
        event->type = MT_EVENT_MONITOR_DISCONNECTED;
}

static void joystick_callback(int jid, int action)
{
    MtEvent *event = new_event();
    event->joystick = jid;

    if (action == GLFW_CONNECTED)
        event->type = MT_EVENT_JOYSTICK_CONNECTED;
    else if (action == GLFW_DISCONNECTED)
        event->type = MT_EVENT_JOYSTICK_DISCONNECTED;
}

static void window_maximize_callback(GLFWwindow *window, int maximized)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->window = win;

    if (maximized)
        event->type = MT_EVENT_WINDOW_MAXIMIZED;
    else
        event->type = MT_EVENT_WINDOW_UNMAXIMIZED;
}

static void window_content_scale_callback(GLFWwindow *window, float xscale, float yscale)
{
    MtWindow *win = glfwGetWindowUserPointer(window);
    MtEvent *event = new_event();
    event->window = win;
    event->type = MT_EVENT_WINDOW_SCALE_CHANGED;
    event->scale.x = xscale;
    event->scale.y = yscale;
}
