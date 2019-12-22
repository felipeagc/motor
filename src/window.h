#ifndef MT_WINDOW_H
#define MT_WINDOW_H

#include <stdint.h>

typedef struct MtWindow MtWindow;

typedef struct MtWindowVT {
  void (*destroy)(MtWindow *);
} MtWindowVT;

typedef struct MtIWindow {
  MtWindow *inst;
  MtWindowVT *vt;
} MtIWindow;

#endif
