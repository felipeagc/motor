#include <motor/engine/systems.h>

#include <motor/base/log.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/engine.h>
#include <motor/engine/scene.h>
#include <motor/engine/camera.h>
#include <motor/engine/environment.h>
#include <motor/engine/entity_archetypes.h>
#include <motor/engine/transform.h>
#include <motor/engine/assets/gltf_asset.h>
#include <motor/engine/assets/pipeline_asset.h>

void mt_light_system(MtEntityArchetype *arch, MtScene *scene, float delta)
{
    MtPointLightArchetype *comps = (MtPointLightArchetype *)arch->components;

    static float acc = 0.0f;
    acc += delta;

    float x = sinf(acc * 2.0f) * 2.0f;
    float z = cosf(acc * 2.0f) * 2.0f;

    MtComponentMask comp_mask =
        MT_COMP_BIT(MtPointLightArchetype, pos) | MT_COMP_BIT(MtPointLightArchetype, color);

    scene->env.uniform.point_light_count = 0;
    for (uint32_t e = 0; e < arch->entity_count; ++e)
    {
        if (!(arch->masks[e] & comp_mask)) continue;

        uint32_t l = scene->env.uniform.point_light_count;

        scene->env.uniform.point_lights[l].pos.xyz = comps->pos[e];
        scene->env.uniform.point_lights[l].pos.x += x;
        scene->env.uniform.point_lights[l].pos.z += z;
        scene->env.uniform.point_lights[l].pos.w = 1.0f;

        scene->env.uniform.point_lights[l].color = comps->color[e];

        scene->env.uniform.point_light_count++;
    }
}

void mt_model_system(MtEntityArchetype *arch, MtScene *scene, MtCmdBuffer *cb)
{
    mt_render.cmd_bind_pipeline(cb, scene->engine->pbr_pipeline->pipeline);
    mt_render.cmd_bind_uniform(cb, &scene->cam.uniform, sizeof(scene->cam.uniform), 0, 0);
    mt_environment_bind(&scene->env, cb, 3);

    MtComponentMask comp_mask =
        MT_COMP_BIT(MtModelArchetype, transform) | MT_COMP_BIT(MtModelArchetype, model);

    MtModelArchetype *comps = (MtModelArchetype *)arch->components;
    for (MtEntity e = 0; e < arch->entity_count; ++e)
    {
        if (!(arch->masks[e] & comp_mask)) continue;

        Mat4 mat = mt_transform_matrix(&comps->transform[e]);

        mt_gltf_asset_draw(comps->model[e], cb, &mat, 1, 2);
    }
}

void mt_selected_model_system(MtEntityArchetype *arch, MtScene *scene, MtCmdBuffer *cb)
{
    MtEngine *engine = scene->engine;
    MtPipeline *gizmo_pipeline = engine->gizmo_pipeline->pipeline;
    MtPipeline *selected_pipeline = engine->wireframe_pipeline->pipeline;
    MtModelArchetype *comps = (MtModelArchetype *)arch->components;

    if (arch->selected_entity != MT_ENTITY_INVALID)
    {
        MtEntity e = arch->selected_entity;

        Mat4 transform = mt_transform_matrix(&comps->transform[e]);

        // Draw wireframe
        mt_render.cmd_bind_pipeline(cb, selected_pipeline);
        mt_render.cmd_bind_uniform(cb, &scene->cam.uniform, sizeof(scene->cam.uniform), 0, 0);
        mt_render.cmd_bind_uniform(cb, &(Vec4){0.7f, 0.17f, 0.27f, 0.5f}, sizeof(Vec4), 2, 0);
        mt_gltf_asset_draw(comps->model[e], cb, &transform, 1, UINT32_MAX);

        mt_render.cmd_bind_pipeline(cb, gizmo_pipeline);
        mt_translation_gizmo_draw(
            &engine->gizmo, cb, engine->window, &scene->cam.uniform, &comps->transform[e].pos);

        if (arch->masks[e] & MT_COMP_BIT(MtModelArchetype, actor))
        {
            MtPhysicsShape *shapes[32];
            uint32_t shape_count = mt_rigid_actor_get_shape_count(comps->actor[e]);
            mt_rigid_actor_get_shapes(comps->actor[e], shapes, shape_count, 0);

            mt_render.cmd_bind_pipeline(cb, scene->engine->wireframe_pipeline->pipeline);
            mt_render.cmd_bind_uniform(cb, &scene->cam.uniform, sizeof(scene->cam.uniform), 0, 0);
            mt_render.cmd_bind_uniform(cb, &V4(0.0f, 1.0f, 0.0f, 1.0f), sizeof(Vec4), 2, 0);

            for (uint32_t i = 0; i < shape_count; ++i)
            {
                float radius = mt_physics_shape_get_radius(shapes[i]);
                MtPhysicsTransform px_transform = mt_physics_shape_get_local_transform(shapes[i]);
                MtTransform shape_transform = {0};
                shape_transform.pos = px_transform.pos;
                shape_transform.rot = px_transform.rot;
                shape_transform.scale = V3(radius, radius, radius);

                MtTransform model_transform = comps->transform[e];
                model_transform.scale = V3(1, 1, 1);

                Mat4 transform = mat4_mul(
                    mt_transform_matrix(&shape_transform), mt_transform_matrix(&model_transform));
                mt_render.cmd_bind_uniform(cb, &transform, sizeof(transform), 1, 0);

                switch (mt_physics_shape_get_type(shapes[i]))
                {
                    case MT_PHYSICS_SHAPE_SPHERE: {
                        mt_mesh_draw(&scene->engine->sphere_mesh, cb);
                        break;
                    }
                    case MT_PHYSICS_SHAPE_PLANE: {
                        break;
                    }
                }
            }
        }
    }
}

void mt_picking_system(MtCmdBuffer *cb, void *user_data)
{
    MtEntityArchetype *arch = user_data;
    MtModelArchetype *comps = (MtModelArchetype *)arch->components;
    MtComponentMask comp_mask =
        MT_COMP_BIT(MtModelArchetype, transform) | MT_COMP_BIT(MtModelArchetype, model);

    for (uint32_t e = 0; e < arch->entity_count; ++e)
    {
        if (!(arch->masks[e] & comp_mask)) continue;

        Mat4 transform = mt_transform_matrix(&comps->transform[e]);

        mt_render.cmd_bind_uniform(cb, &e, sizeof(uint32_t), 2, 0);
        mt_gltf_asset_draw(comps->model[e], cb, &transform, 1, UINT32_MAX);
    }
}

void mt_pre_physics_sync_system(MtEntityArchetype *arch)
{
    MtComponentMask comp_mask =
        MT_COMP_BIT(MtModelArchetype, transform) | MT_COMP_BIT(MtModelArchetype, actor);

    MtModelArchetype *comps = (MtModelArchetype *)arch->components;
    for (MtEntity e = 0; e < arch->entity_count; ++e)
    {
        if (!(arch->masks[e] & comp_mask)) continue;

        mt_rigid_actor_set_transform(
            comps->actor[e],
            &(MtPhysicsTransform){.pos = comps->transform[e].pos, .rot = comps->transform[e].rot});
    }
}

void mt_post_physics_sync_system(MtEntityArchetype *arch)
{
    MtComponentMask comp_mask =
        MT_COMP_BIT(MtModelArchetype, transform) | MT_COMP_BIT(MtModelArchetype, actor);

    MtModelArchetype *comps = (MtModelArchetype *)arch->components;
    for (MtEntity e = 0; e < arch->entity_count; ++e)
    {
        if (!(arch->masks[e] & comp_mask)) continue;

        MtPhysicsTransform transform = mt_rigid_actor_get_transform(comps->actor[e]);
        comps->transform[e].pos = transform.pos;
        comps->transform[e].rot = transform.rot;
    }
}
