#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct MtDevice MtDevice;
typedef struct MtAllocator MtAllocator;

typedef struct MtWindow MtWindow;

typedef struct MtRenderPass MtRenderPass;
typedef struct MtCmdBuffer MtCmdBuffer;

typedef enum MtCursorMode
{
    MT_CURSOR_MODE_NORMAL,
    MT_CURSOR_MODE_HIDDEN,
    MT_CURSOR_MODE_DISABLED,
} MtCursorMode;

typedef enum MtCursorType
{
    MT_CURSOR_TYPE_ARROW,
    MT_CURSOR_TYPE_IBEAM,
    MT_CURSOR_TYPE_CROSSHAIR,
    MT_CURSOR_TYPE_HAND,
    MT_CURSOR_TYPE_HRESIZE,
    MT_CURSOR_TYPE_VRESIZE,
    MT_CURSOR_TYPE_MAX,
} MtCursorType;

typedef enum MtInputState
{
    MT_INPUT_STATE_RELEASE,
    MT_INPUT_STATE_PRESS,
    MT_INPUT_STATE_REPEAT,
} MtInputState;

typedef enum MtMouseButton
{
    MT_MOUSE_BUTTON1       = 0,
    MT_MOUSE_BUTTON2       = 1,
    MT_MOUSE_BUTTON3       = 2,
    MT_MOUSE_BUTTON4       = 3,
    MT_MOUSE_BUTTON5       = 4,
    MT_MOUSE_BUTTON6       = 5,
    MT_MOUSE_BUTTON7       = 6,
    MT_MOUSE_BUTTON8       = 7,
    MT_MOUSE_BUTTON_LEFT   = MT_MOUSE_BUTTON1,
    MT_MOUSE_BUTTON_RIGHT  = MT_MOUSE_BUTTON2,
    MT_MOUSE_BUTTON_MIDDLE = MT_MOUSE_BUTTON3,
} MtMouseButton;

typedef enum MtEventType
{
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
    union {
        MtWindow *window;
        void *monitor;
        int32_t joystick;
    };
    union {
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
    void (*poll_events)(void);
    void (*destroy_window_system)(void);

    MtWindow *(*create)(
        MtDevice *, uint32_t width, uint32_t height, const char *title, MtAllocator *alloc);
    void (*destroy)(MtWindow *);

    bool (*should_close)(MtWindow *);
    bool (*next_event)(MtWindow *, MtEvent *);

    MtCmdBuffer *(*begin_frame)(MtWindow *);
    void (*end_frame)(MtWindow *);
    MtRenderPass *(*get_render_pass)(MtWindow *);

    float (*delta_time)(MtWindow *);
    void (*get_size)(MtWindow *, uint32_t *width, uint32_t *height);

    void (*get_cursor_pos)(MtWindow *, double *x, double *y);
    void (*set_cursor_pos)(MtWindow *, double x, double y);

    MtCursorMode (*get_cursor_mode)(MtWindow *);
    void (*set_cursor_mode)(MtWindow *, MtCursorMode);

    void (*set_cursor_type)(MtWindow *, MtCursorType);

    MtInputState (*get_key)(MtWindow *, uint32_t key_code);
    MtInputState (*get_mouse_button)(MtWindow *, MtMouseButton button);
} MtWindowSystem;

extern MtWindowSystem mt_window;
