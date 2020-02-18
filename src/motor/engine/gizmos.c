#include <motor/engine/gizmos.h>

#include <string.h>
#include <motor/base/math.h>
#include <motor/base/log.h>
#include <motor/graphics/renderer.h>
#include "common_geometry.inl"

void mt_draw_translation_gizmo(MtCmdBuffer *cb, const Vec3 *pos)
{
    struct
    {
        Mat4 model;
        Vec4 color;
    } uniform;

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

        mt_render.cmd_bind_uniform(cb, &uniform, sizeof(uniform), 1, 0);
        void *mapping = mt_render.cmd_bind_vertex_data(cb, sizeof(g_cube_vertices));
        memcpy(mapping, g_cube_vertices, sizeof(g_cube_vertices));
        mt_render.cmd_draw(cb, 36, 1, 0, 0);
    }
}
