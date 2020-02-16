#pragma once

#include "api_types.h"
#include <motor/base/math_types.h>

#ifdef __cpluspus
extern "C" {
#endif

typedef struct MtWindow MtWindow;
typedef struct MtEvent MtEvent;

typedef struct MtCameraUniform
{
    Mat4 view;
    Mat4 proj;
    Vec4 pos;
} MtCameraUniform;

typedef struct MtPerspectiveCamera
{
    MtCameraUniform uniform;

    float yaw;
    float pitch;

    Vec3 pos;

    float speed;

    float near;
    float far;
    float fovy;

    double prev_x, prev_y;

    float sensitivity;
} MtPerspectiveCamera;

MT_ENGINE_API void mt_perspective_camera_init(MtPerspectiveCamera *c);

MT_ENGINE_API void mt_perspective_camera_on_event(MtPerspectiveCamera *c, MtEvent *event);

MT_ENGINE_API void
mt_perspective_camera_update(MtPerspectiveCamera *c, MtWindow *win, float aspect, float delta_time);

#ifdef __cpluspus
}
#endif
