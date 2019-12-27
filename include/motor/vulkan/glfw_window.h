#pragma once

#include <stdint.h>

typedef struct MtArena MtArena;
typedef struct MtDevice MtDevice;
typedef struct MtIWindow MtIWindow;
typedef struct MtIWindowSystem MtIWindowSystem;

void mt_glfw_vulkan_init(MtIWindowSystem *system);

void mt_glfw_vulkan_window_init(
    MtIWindow *window,
    MtDevice *dev,
    uint32_t width,
    uint32_t height,
    const char *title,
    MtArena *arena);
