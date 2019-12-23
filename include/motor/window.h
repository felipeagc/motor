#ifndef MT_WINDOW_H
#define MT_WINDOW_H

#include <stdint.h>
#include <stdbool.h>

typedef struct MtWindow MtWindow;
typedef struct MtWindowSystem MtWindowSystem;

typedef struct MtRenderPass MtRenderPass;
typedef struct MtICmdBuffer MtICmdBuffer;

typedef struct MtEvent MtEvent;

typedef struct MtWindowSystemVT {
  void (*poll_events)(void);
  void (*destroy)(void);
} MtWindowSystemVT;

typedef struct MtIWindowSystem {
  MtWindowSystem *inst;
  MtWindowSystemVT *vt;
} MtIWindowSystem;

typedef struct MtWindowVT {
  bool (*should_close)(MtWindow *);
  bool (*next_event)(MtWindow *, MtEvent *);

  MtICmdBuffer (*begin_frame)(MtWindow *);
  void (*end_frame)(MtWindow *);
  MtRenderPass *(*get_render_pass)(MtWindow *);

  void (*destroy)(MtWindow *);
} MtWindowVT;

typedef struct MtIWindow {
  MtWindow *inst;
  MtWindowVT *vt;
} MtIWindow;

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

typedef struct MtEvent {
  MtEventType type;
  union {
    MtIWindow *window;
    void *monitor;
    int32_t joystick;
  };
  union {
    struct {
      int32_t x;
      int32_t y;
    } pos;
    struct {
      int32_t width;
      int32_t height;
    } size;
    struct {
      double x;
      double y;
    } scroll;
    struct {
      int32_t key;
      int32_t scancode;
      int32_t mods;
    } keyboard;
    struct {
      int32_t button;
      int32_t mods;
    } mouse;
    uint32_t codepoint;
    struct {
      float x;
      float y;
    } scale;
  };
} MtEvent;

#endif
