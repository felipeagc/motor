#include <motor/engine/gizmos.h>

#include <string.h>
#include <motor/base/math.h>
#include <motor/base/log.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/picker.h>
#include <motor/engine/camera.h>
#include "common_geometry.inl"

#define GIZMO_RADIUS 0.05f

static Ray get_mouse_ray(MtWindow *window, const MtCameraUniform *camera)
{
    float x, y;
    mt_window.get_cursor_pos_normalized(window, &x, &y);

    Mat4 view_proj = mat4_mul(camera->view, camera->proj);
    Mat4 inv_view_proj = mat4_inverse(view_proj);

    Vec4 world_start = mat4_mulv(inv_view_proj, V4(x, y, 0.0f, 1.0f));
    world_start = v4_muls(world_start, 1.f / world_start.w);

    Vec4 world_end = mat4_mulv(inv_view_proj, V4(x, y, 1.0f, 1.0f));
    world_end = v4_muls(world_end, 1.f / world_end.w);

    return (Ray){
        .origin = world_start.xyz,
        .direction = v3_sub(world_end.xyz, world_start.xyz),
        .t = FLT_MAX,
    };
}

void mt_translation_gizmo_draw(
    MtGizmo *gizmo, MtCmdBuffer *cb, MtWindow *window, const MtCameraUniform *camera, Vec3 *pos)
{
    gizmo->previous_intersect = gizmo->current_intersect;

    Ray mouse_ray = get_mouse_ray(window, camera);

    Ray axis_rays[3];
    axis_rays[0] = (Ray){.origin = *pos, .direction = V3(1.f, 0.f, 0.f)};
    axis_rays[1] = (Ray){.origin = *pos, .direction = V3(0.f, 1.f, 0.f)};
    axis_rays[2] = (Ray){.origin = *pos, .direction = V3(0.f, 0.f, 1.f)};
    MtGizmoState states[3] = {
        MT_GIZMO_STATE_TRANSLATE_X, MT_GIZMO_STATE_TRANSLATE_Y, MT_GIZMO_STATE_TRANSLATE_Z};

    MtGizmoState state = MT_GIZMO_STATE_NONE;

    for (uint32_t i = 0; i < 3; ++i)
    {
        float dist = ray_distance(&mouse_ray, &axis_rays[i]);
        bool hovering = dist < GIZMO_RADIUS && axis_rays[i].t >= -1.0f && axis_rays[i].t < 0.0f;
        bool pressing =
            mt_window.get_mouse_button(window, MT_MOUSE_BUTTON_LEFT) == MT_INPUT_STATE_PRESS;
        if ((hovering || gizmo->state == states[i]) && pressing)
        {
            gizmo->current_intersect =
                v3_add(axis_rays[i].origin, v3_muls(axis_rays[i].direction, -axis_rays[i].t));

            if (gizmo->state == states[i])
            {
                Vec3 delta = v3_sub(gizmo->current_intersect, gizmo->previous_intersect);
                *pos = v3_add(*pos, delta);
            }

            state = states[i];
        }
    }

    gizmo->state = state;

    struct
    {
        MtCameraUniform cam;
        Mat4 model;
        Vec4 color;
    } uniform;
    uniform.cam = *camera;

    Vec4 colors[3];
    colors[0] = V4(1, 0, 0, 1);
    colors[1] = V4(0, 1, 0, 1);
    colors[2] = V4(0, 0, 1, 1);

    Vec3 translations[3];
    translations[0] = V3(0.5, 0.0, 0.0);
    translations[1] = V3(0.0, 0.5, 0.0);
    translations[2] = V3(0.0, 0.0, 0.5);

    Vec3 scales[3];
    scales[0] = V3(1, 0.05, 0.05);
    scales[1] = V3(0.05, 1, 0.05);
    scales[2] = V3(0.05, 0.05, 1);

    for (uint32_t i = 0; i < 3; ++i)
    {
        uniform.model = mat4_identity();
        uniform.model = mat4_translate(uniform.model, *pos);
        uniform.model = mat4_translate(uniform.model, translations[i]);
        uniform.model = mat4_scale(uniform.model, scales[i]);

        uniform.color = colors[i];

        mt_render.cmd_bind_uniform(cb, &uniform, sizeof(uniform), 0, 0);

        void *mapping = mt_render.cmd_bind_vertex_data(cb, sizeof(g_cube_vertices));
        memcpy(mapping, g_cube_vertices, sizeof(g_cube_vertices));

        mt_render.cmd_draw(cb, 36, 1, 0, 0);
    }
}
