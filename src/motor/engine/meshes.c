#include <motor/engine/meshes.h>

#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/engine.h>

void mt_mesh_init_sphere(MtMesh *mesh, MtEngine *engine)
{
    memset(mesh, 0, sizeof(*mesh));
    mesh->engine = engine;

    MtStandardVertex *vertices = NULL;

    float s = (MT_PI * 2.0f) / 40.0f;

    for (float h = 0.0f; h < MT_PI * 2.0f; h += s)
    {
        for (float v = -MT_PI / 2.0f; v < MT_PI / 2.0f; v += s)
        {
            MtStandardVertex v1 = {.pos = V3(cosf(v) * cosf(h), sinf(v), cosf(v) * sinf(h))};
            MtStandardVertex v2 = {
                .pos = V3(cosf(v + s) * cosf(h), sinf(v + s), cosf(v + s) * sinf(h))};
            MtStandardVertex v3 = {
                .pos = V3(cosf(v + s) * cosf(h + s), sinf(v + s), cosf(v + s) * sinf(h + s))};

            MtStandardVertex v4 = {
                .pos = V3(cosf(v + s) * cosf(h + s), sinf(v + s), cosf(v + s) * sinf(h + s))};
            MtStandardVertex v5 = {.pos =
                                       V3(cosf(v) * cosf(h + s), sinf(v), cosf(v) * sinf(h + s))};
            MtStandardVertex v6 = {.pos = V3(cosf(v) * cosf(h), sinf(v), cosf(v) * sinf(h))};

            mt_array_push(engine->alloc, vertices, v1);
            mt_array_push(engine->alloc, vertices, v2);
            mt_array_push(engine->alloc, vertices, v3);
            mt_array_push(engine->alloc, vertices, v4);
            mt_array_push(engine->alloc, vertices, v5);
            mt_array_push(engine->alloc, vertices, v6);
        }
    }

    mesh->vertex_count = mt_array_size(vertices);

    mesh->vertex_buffer = mt_render.create_buffer(
        engine->device,
        &(MtBufferCreateInfo){
            .usage = MT_BUFFER_USAGE_VERTEX,
            .memory = MT_BUFFER_MEMORY_DEVICE,
            .size = sizeof(MtStandardVertex) * mt_array_size(vertices),
        });

    mt_render.transfer_to_buffer(
        engine->device,
        mesh->vertex_buffer,
        0,
        sizeof(MtStandardVertex) * mt_array_size(vertices),
        vertices);

    mt_array_free(engine->alloc, vertices);
}

void mt_mesh_draw(MtMesh *mesh, MtCmdBuffer *cb)
{
    mt_render.cmd_bind_vertex_buffer(cb, mesh->vertex_buffer, 0);
    mt_render.cmd_draw(cb, mesh->vertex_count, 1, 0, 0);
}

void mt_mesh_destroy(MtMesh *mesh)
{
    mt_render.destroy_buffer(mesh->engine->device, mesh->vertex_buffer);
}
