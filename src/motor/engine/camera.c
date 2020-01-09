#include <motor/engine/camera.h>

#include <motor/base/math.h>
#include <motor/graphics/window.h>
#include <motor/graphics/renderer.h>
#include <string.h>

void mt_perspective_camera_init(MtPerspectiveCamera *c)
{
    memset(c, 0, sizeof(*c));

    c->yaw   = 0.0f;
    c->pitch = 0.0f;

    c->pos   = V3(0.0f, 0.0f, 0.0f);
    c->speed = 5.0f;

    c->near = 0.1f;
    c->far  = 300.0f;

    c->fovy = 75.0f * (MT_PI / 180.0f);

    c->prev_x = 0.0f;
    c->prev_y = 0.0f;

    c->sensitivity = 0.07f;
}

void mt_perspective_camera_on_event(MtPerspectiveCamera *c, MtEvent *event)
{
    switch (event->type)
    {
        case MT_EVENT_BUTTON_PRESSED:
        {
            if (event->mouse.button == MT_MOUSE_BUTTON_RIGHT)
            {
                mt_window.set_cursor_mode(event->window, MT_CURSOR_MODE_DISABLED);
            }
            break;
        }
        case MT_EVENT_BUTTON_RELEASED:
        {
            if (event->mouse.button == MT_MOUSE_BUTTON_RIGHT)
            {
                mt_window.set_cursor_mode(event->window, MT_CURSOR_MODE_NORMAL);
            }
            break;
        }
        default: break;
    }
}

void mt_perspective_camera_update(MtPerspectiveCamera *c, MtWindow *win, float aspect)
{
    double x, y;
    mt_window.get_cursor_pos(win, &x, &y);

    double dx = x - c->prev_x;
    double dy = y - c->prev_y;

    c->prev_x = x;
    c->prev_y = y;

    if (mt_window.get_cursor_mode(win) == MT_CURSOR_MODE_DISABLED)
    {
        c->yaw += dx * c->sensitivity * (MT_PI / 180.0f);
        c->pitch -= dy * c->sensitivity * (MT_PI / 180.0f);
        c->pitch = clamp(c->pitch, -89.0f * (MT_PI / 180.0f), 89.0f * (MT_PI / 180.0f));
    }

    Vec3 front;
    front.x = sin(c->yaw) * cos(c->pitch);
    front.y = sin(c->pitch);
    front.z = cos(c->yaw) * cos(c->pitch);
    front   = v3_normalize(front);

    Vec3 right = v3_normalize(v3_cross(front, V3(0.0f, -1.0f, 0.0f)));
    Vec3 up    = v3_normalize(v3_cross(right, front));

    float delta = c->speed * (float)mt_window.delta_time(win);
    if (mt_window.get_key(win, 'W'))
    {
        c->pos = v3_add(c->pos, v3_muls(front, delta));
    }
    if (mt_window.get_key(win, 'S'))
    {
        c->pos = v3_sub(c->pos, v3_muls(front, delta));
    }
    if (mt_window.get_key(win, 'A'))
    {
        c->pos = v3_sub(c->pos, v3_muls(right, delta));
    }
    if (mt_window.get_key(win, 'D'))
    {
        c->pos = v3_add(c->pos, v3_muls(right, delta));
    }

    c->uniform.pos.xyz = c->pos;
    c->uniform.pos.w   = 1.0f;

    c->uniform.view = mat4_look_at(c->pos, v3_add(c->pos, front), up);

    c->uniform.proj = mat4_perspective(c->fovy, aspect, c->near, c->far);
}
