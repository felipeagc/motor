#pragma once

#include <motor/base/math_types.h>

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

void mt_perspective_camera_init(MtPerspectiveCamera *c);

void mt_perspective_camera_on_event(MtPerspectiveCamera *c, MtEvent *event);

void mt_perspective_camera_update(
    MtPerspectiveCamera *c, MtWindow *win, float aspect, float delta_time);
