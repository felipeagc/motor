#ifndef MT_WINDOW_H
#define MT_WINDOW_H

#include <stdint.h>
#include <stdbool.h>

typedef struct MtWindow MtWindow;
typedef struct MtWindowSystem MtWindowSystem;

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
  void (*destroy)(MtWindow *);
} MtWindowVT;

typedef struct MtIWindow {
  MtWindow *inst;
  MtWindowVT *vt;
} MtIWindow;

#endif
