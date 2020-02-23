#include <motor/engine/gizmos.h>

#include <string.h>
#include <motor/base/math.h>
#include <motor/base/log.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/picker.h>
#include <motor/engine/camera.h>
#include "common_geometry.inl"

#define GIZMO_RADIUS 0.1f
#define GIZMO_SELECTED_COLOR V4(1.f, 0.8f, 0.f, 1.f)

static Ray get_mouse_ray(MtWindow *window, const MtCameraUniform *camera)
{
    float x, y;
    mt_window.get_cursor_pos_normalized(window, &x, &y);

    Mat4 view_proj = mat4_mul(camera->view, camera->proj);
    Mat4 inv_view_proj = mat4_inverse(view_proj);

    Vec4 world_start = mat4_mulv(inv_view_proj, V4(x, y, 0.f, 1.f));
    world_start = v4_muls(world_start, 1.f / world_start.w);

    Vec4 world_end = mat4_mulv(inv_view_proj, V4(x, y, 1.f, 1.f));
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

    float dists[3];
    dists[0] = ray_distance(&mouse_ray, &axis_rays[0]);
    dists[1] = ray_distance(&mouse_ray, &axis_rays[1]);
    dists[2] = ray_distance(&mouse_ray, &axis_rays[2]);

    gizmo->hovered = false;

    MtGizmoState state = MT_GIZMO_STATE_NONE;
    uint32_t axis = UINT32_MAX;

    bool pressing =
        mt_window.get_mouse_button(window, MT_MOUSE_BUTTON_LEFT) == MT_INPUT_STATE_PRESS;

    for (uint32_t i = 0; i < 3; ++i)
    {
        bool hovering = dists[i] < GIZMO_RADIUS && axis_rays[i].t >= -1.f && axis_rays[i].t < 0.f;

        if (hovering && !pressing)
        {
            gizmo->hovered = true;
            axis = i;
            break;
        }
    }

    for (uint32_t i = 0; i < 3; ++i)
    {
        bool hovering = dists[i] < GIZMO_RADIUS && axis_rays[i].t >= -1.f && axis_rays[i].t < 0.f;

        if (((hovering && gizmo->axis == i) ||
             (gizmo->state == MT_GIZMO_STATE_TRANSLATE && gizmo->axis == i)) &&
            pressing)
        {
            axis = i;
            state = MT_GIZMO_STATE_TRANSLATE;

            gizmo->current_intersect =
                v3_add(axis_rays[i].origin, v3_muls(axis_rays[i].direction, -axis_rays[i].t));

            if (gizmo->state == MT_GIZMO_STATE_TRANSLATE)
            {
                Vec3 delta = v3_sub(gizmo->current_intersect, gizmo->previous_intersect);
                *pos = v3_add(*pos, delta);
            }
            break;
        }
    }

    gizmo->state = state;
    gizmo->axis = axis;

    struct
    {
        MtCameraUniform cam;
        Mat4 model;
        Vec4 color;
    } uniform;
    uniform.cam = *camera;

    Vec4 colors[3];
    colors[0] = V4(1.f, 0.f, 0.f, 1.f);
    colors[1] = V4(0.f, 1.f, 0.f, 1.f);
    colors[2] = V4(0.f, 0.f, 1.f, 1.f);

    struct
    {
        Vec3 start;
        Vec3 end;
    } lines[3];

    lines[0].start = V3(0.f, 0.f, 0.f);
    lines[0].end = V3(1.f, 0.f, 0.f);

    lines[1].start = V3(0.f, 0.f, 0.f);
    lines[1].end = V3(0.f, 1.f, 0.f);

    lines[2].start = V3(0.f, 0.f, 0.f);
    lines[2].end = V3(0.f, 0.f, 1.f);

    for (uint32_t i = 0; i < 3; ++i)
    {
        uniform.model = mat4_translate(mat4_identity(), *pos);
        uniform.color = colors[i];

        if (gizmo->axis == i)
        {
            uniform.color = GIZMO_SELECTED_COLOR;
        }

        mt_render.cmd_bind_uniform(cb, &uniform, sizeof(uniform), 0, 0);

        void *mapping = mt_render.cmd_bind_vertex_data(cb, sizeof(lines[i]));
        memcpy(mapping, &lines[i], sizeof(lines[i]));

        mt_render.cmd_draw(cb, 2, 1, 0, 0);
    }
}
