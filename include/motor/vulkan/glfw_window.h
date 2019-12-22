#ifndef MT_GLFW_WINDOW_H
#define MT_GLFW_WINDOW_H

#include <stdint.h>

typedef struct MtArena MtArena;
typedef struct MtIWindow MtIWindow;
typedef struct MtIWindowSystem MtIWindowSystem;

void mt_glfw_vulkan_init(MtIWindowSystem *system);

void mt_glfw_vulkan_window_init(
    MtIWindow *window,
    uint32_t width,
    uint32_t height,
    const char *title,
    MtArena *arena);

#endif
