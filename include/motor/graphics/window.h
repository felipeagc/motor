#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
#if defined(_WIN32)
#elif defined(__APPLE__)
    #error Apple not supported
#else
    #include <X11/Xlib.h>
#endif
// clang-format on

typedef struct MtDevice MtDevice;
typedef struct MtAllocator MtAllocator;

typedef struct MtWindow MtWindow;

typedef struct MtRenderPass MtRenderPass;
typedef struct MtCmdBuffer MtCmdBuffer;

typedef enum MtCursorMode {
    MT_CURSOR_MODE_NORMAL,
    MT_CURSOR_MODE_HIDDEN,
    MT_CURSOR_MODE_DISABLED,
} MtCursorMode;

typedef enum MtCursorType {
    MT_CURSOR_TYPE_ARROW,
    MT_CURSOR_TYPE_IBEAM,
    MT_CURSOR_TYPE_CROSSHAIR,
    MT_CURSOR_TYPE_HAND,
    MT_CURSOR_TYPE_HRESIZE,
    MT_CURSOR_TYPE_VRESIZE,
    MT_CURSOR_TYPE_MAX,
} MtCursorType;

typedef enum MtInputState {
    MT_INPUT_STATE_RELEASE = 0,
    MT_INPUT_STATE_PRESS = 1,
    MT_INPUT_STATE_REPEAT = 2,
} MtInputState;

typedef enum MtMouseButton {
    MT_MOUSE_BUTTON1 = 0,
    MT_MOUSE_BUTTON2 = 1,
    MT_MOUSE_BUTTON3 = 2,
    MT_MOUSE_BUTTON4 = 3,
    MT_MOUSE_BUTTON5 = 4,
    MT_MOUSE_BUTTON6 = 5,
    MT_MOUSE_BUTTON7 = 6,
    MT_MOUSE_BUTTON8 = 7,
    MT_MOUSE_BUTTON_LEFT = MT_MOUSE_BUTTON1,
    MT_MOUSE_BUTTON_RIGHT = MT_MOUSE_BUTTON2,
    MT_MOUSE_BUTTON_MIDDLE = MT_MOUSE_BUTTON3,
} MtMouseButton;

typedef enum MtKey {
    MT_KEY_UNKNOWN = -1,

    /* Printable keys */
    MT_KEY_SPACE = 32,
    MT_KEY_APOSTROPHE = 39,
    MT_KEY_COMMA = 44,
    MT_KEY_MINUS = 45,
    MT_KEY_PERIOD = 46,
    MT_KEY_SLASH = 47,
    MT_KEY_0 = 48,
    MT_KEY_1 = 49,
    MT_KEY_2 = 50,
    MT_KEY_3 = 51,
    MT_KEY_4 = 52,
    MT_KEY_5 = 53,
    MT_KEY_6 = 54,
    MT_KEY_7 = 55,
    MT_KEY_8 = 56,
    MT_KEY_9 = 57,
    MT_KEY_SEMICOLON = 59,
    MT_KEY_EQUAL = 61,
    MT_KEY_A = 65,
    MT_KEY_B = 66,
    MT_KEY_C = 67,
    MT_KEY_D = 68,
    MT_KEY_E = 69,
    MT_KEY_F = 70,
    MT_KEY_G = 71,
    MT_KEY_H = 72,
    MT_KEY_I = 73,
    MT_KEY_J = 74,
    MT_KEY_K = 75,
    MT_KEY_L = 76,
    MT_KEY_M = 77,
    MT_KEY_N = 78,
    MT_KEY_O = 79,
    MT_KEY_P = 80,
    MT_KEY_Q = 81,
    MT_KEY_R = 82,
    MT_KEY_S = 83,
    MT_KEY_T = 84,
    MT_KEY_U = 85,
    MT_KEY_V = 86,
    MT_KEY_W = 87,
    MT_KEY_X = 88,
    MT_KEY_Y = 89,
    MT_KEY_Z = 90,
    MT_KEY_LEFT_BRACKET = 91,
    MT_KEY_BACKSLASH = 92,
    MT_KEY_RIGHT_BRACKET = 93,
    MT_KEY_GRAVE_ACCENT = 96,
    MT_KEY_WORLD_1 = 161,
    MT_KEY_WORLD_2 = 162,

    /* Function keys */
    MT_KEY_ESCAPE = 256,
    MT_KEY_ENTER = 257,
    MT_KEY_TAB = 258,
    MT_KEY_BACKSPACE = 259,
    MT_KEY_INSERT = 260,
    MT_KEY_DELETE = 261,
    MT_KEY_RIGHT = 262,
    MT_KEY_LEFT = 263,
    MT_KEY_DOWN = 264,
    MT_KEY_UP = 265,
    MT_KEY_PAGE_UP = 266,
    MT_KEY_PAGE_DOWN = 267,
    MT_KEY_HOME = 268,
    MT_KEY_END = 269,
    MT_KEY_CAPS_LOCK = 280,
    MT_KEY_SCROLL_LOCK = 281,
    MT_KEY_NUM_LOCK = 282,
    MT_KEY_PRINT_SCREEN = 283,
    MT_KEY_PAUSE = 284,
    MT_KEY_F1 = 290,
    MT_KEY_F2 = 291,
    MT_KEY_F3 = 292,
    MT_KEY_F4 = 293,
    MT_KEY_F5 = 294,
    MT_KEY_F6 = 295,
    MT_KEY_F7 = 296,
    MT_KEY_F8 = 297,
    MT_KEY_F9 = 298,
    MT_KEY_F10 = 299,
    MT_KEY_F11 = 300,
    MT_KEY_F12 = 301,
    MT_KEY_F13 = 302,
    MT_KEY_F14 = 303,
    MT_KEY_F15 = 304,
    MT_KEY_F16 = 305,
    MT_KEY_F17 = 306,
    MT_KEY_F18 = 307,
    MT_KEY_F19 = 308,
    MT_KEY_F20 = 309,
    MT_KEY_F21 = 310,
    MT_KEY_F22 = 311,
    MT_KEY_F23 = 312,
    MT_KEY_F24 = 313,
    MT_KEY_F25 = 314,
    MT_KEY_KP_0 = 320,
    MT_KEY_KP_1 = 321,
    MT_KEY_KP_2 = 322,
    MT_KEY_KP_3 = 323,
    MT_KEY_KP_4 = 324,
    MT_KEY_KP_5 = 325,
    MT_KEY_KP_6 = 326,
    MT_KEY_KP_7 = 327,
    MT_KEY_KP_8 = 328,
    MT_KEY_KP_9 = 329,
    MT_KEY_KP_DECIMAL = 330,
    MT_KEY_KP_DIVIDE = 331,
    MT_KEY_KP_MULTIPLY = 332,
    MT_KEY_KP_SUBTRACT = 333,
    MT_KEY_KP_ADD = 334,
    MT_KEY_KP_ENTER = 335,
    MT_KEY_KP_EQUAL = 336,
    MT_KEY_LEFT_SHIFT = 340,
    MT_KEY_LEFT_CONTROL = 341,
    MT_KEY_LEFT_ALT = 342,
    MT_KEY_LEFT_SUPER = 343,
    MT_KEY_RIGHT_SHIFT = 344,
    MT_KEY_RIGHT_CONTROL = 345,
    MT_KEY_RIGHT_ALT = 346,
    MT_KEY_RIGHT_SUPER = 347,
    MT_KEY_MENU = 348,

    MT_KEY_LAST = MT_KEY_MENU,
} MtKey;

typedef enum MtEventType {
    MT_EVENT_NONE,
    MT_EVENT_WINDOW_MOVED,
    MT_EVENT_WINDOW_RESIZED,
    MT_EVENT_WINDOW_CLOSED,
    MT_EVENT_WINDOW_REFRESH,
    MT_EVENT_WINDOW_FOCUSED,
    MT_EVENT_WINDOW_DEFOCUSED,
    MT_EVENT_WINDOW_ICONIFIED,
    MT_EVENT_WINDOW_UNICONIFIED,
    MT_EVENT_FRAMEBUFFER_RESIZED,
    MT_EVENT_BUTTON_PRESSED,
    MT_EVENT_BUTTON_RELEASED,
    MT_EVENT_CURSOR_MOVED,
    MT_EVENT_CURSOR_ENTERED,
    MT_EVENT_CURSOR_LEFT,
    MT_EVENT_SCROLLED,
    MT_EVENT_KEY_PRESSED,
    MT_EVENT_KEY_REPEATED,
    MT_EVENT_KEY_RELEASED,
    MT_EVENT_CODEPOINT_INPUT,
    MT_EVENT_MONITOR_CONNECTED,
    MT_EVENT_MONITOR_DISCONNECTED,
    MT_EVENT_JOYSTICK_CONNECTED,
    MT_EVENT_JOYSTICK_DISCONNECTED,
    MT_EVENT_WINDOW_MAXIMIZED,
    MT_EVENT_WINDOW_UNMAXIMIZED,
    MT_EVENT_WINDOW_SCALE_CHANGED,
} MtEventType;

typedef struct MtEvent
{
    MtEventType type;
    union
    {
        MtWindow *window;
        void *monitor;
        int32_t joystick;
    };
    union
    {
        struct
        {
            int32_t x;
            int32_t y;
        } pos;
        struct
        {
            int32_t width;
            int32_t height;
        } size;
        struct
        {
            double x;
            double y;
        } scroll;
        struct
        {
            int32_t key;
            int32_t scancode;
            int32_t mods;
        } keyboard;
        struct
        {
            int32_t button;
            int32_t mods;
        } mouse;
        uint32_t codepoint;
        struct
        {
            float x;
            float y;
        } scale;
    };
} MtEvent;

typedef struct MtWindowSystem
{
    // Window system functions:
    void (*poll_events)(void);
    void (*destroy_window_system)(void);

    // Window functions:
    MtWindow *(*create)(uint32_t width, uint32_t height, const char *title, MtAllocator *alloc);
    void (*destroy)(MtWindow *);

    bool (*should_close)(MtWindow *);
    void (*wait_events)(MtWindow *);
    bool (*next_event)(MtWindow *, MtEvent *);

    void (*get_size)(MtWindow *, uint32_t *width, uint32_t *height);

    void (*get_cursor_pos)(MtWindow *, int32_t *x, int32_t *y);
    void (*set_cursor_pos)(MtWindow *, int32_t x, int32_t y);

    void (*get_cursor_pos_normalized)(MtWindow *, float *x, float *y);

    MtCursorMode (*get_cursor_mode)(MtWindow *);
    void (*set_cursor_mode)(MtWindow *, MtCursorMode);

    void (*set_cursor_type)(MtWindow *, MtCursorType);

    MtInputState (*get_key)(MtWindow *, MtKey key_code);
    MtInputState (*get_mouse_button)(MtWindow *, MtMouseButton button);

#if defined(_WIN32)
    void *(*get_win32_window)(MtWindow *);
#elif defined(__APPLE__)
#error Apple not supported
#else
    Window (*get_x11_window)(MtWindow *);
    Display *(*get_x11_display)(MtWindow *);
#endif
} MtWindowSystem;

MT_GRAPHICS_API extern MtWindowSystem mt_window;

#ifdef __cplusplus
}
#endif
