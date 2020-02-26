#include <motor/base/api_types.h>
#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/base/math.h>
#include <motor/base/rand.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/file_watcher.h>
#include <motor/engine/engine.h>
#include <motor/engine/scene.h>
#include <motor/engine/camera.h>
#include <motor/engine/environment.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/entities.h>
#include <motor/engine/entity_archetypes.h>
#include <motor/engine/imgui_impl.h>
#include <motor/engine/inspector.h>
#include <motor/engine/picker.h>
#include <motor/engine/systems.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct Game
{
    /*base*/ MtScene scene;

    MtEntityArchetype *model_archetype;
    MtEntityArchetype *light_archetype;
} Game;

// Color render graph {{{
static void color_pass_builder(MtRenderGraph *graph, MtCmdBuffer *cb, void *user_data)
{
    Game *g = user_data;

    // Draw skybox
    mt_render.cmd_bind_uniform(cb, &g->scene.cam.uniform, sizeof(g->scene.cam.uniform), 0, 0);
    mt_environment_draw_skybox(&g->scene.env, cb);

    // Draw models
    mt_model_system(g->model_archetype, &g->scene, cb);
    mt_selected_model_system(g->model_archetype, &g->scene, cb);

    // Draw UI
    mt_imgui_render(g->scene.engine->imgui_ctx, cb);
}

static void graph_builder(MtRenderGraph *graph, void *user_data)
{
    Game *g = user_data;

    uint32_t width, height;
    mt_window.get_size(g->scene.engine->window, &width, &height);

    MtImageCreateInfo depth_info = {
        .width = width,
        .height = height,
        .format = MT_FORMAT_D32_SFLOAT,
    };

    mt_render.graph_add_image(graph, "depth", &depth_info);

    {
        MtRenderGraphPass *color_pass =
            mt_render.graph_add_pass(graph, "color_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
        mt_render.pass_write(color_pass, MT_PASS_WRITE_DEPTH_STENCIL_ATTACHMENT, "depth");
        mt_render.pass_set_builder(color_pass, color_pass_builder);
    }
}
// }}}

// game_init {{{
static void game_init(MtScene *inst)
{
    Game *g = (Game *)inst;

    MtEngine *engine = inst->engine;
    MtEntityManager *em = inst->entity_manager;
    MtAssetManager *am = inst->asset_manager;

    MtImageAsset *skybox_asset = NULL;
    mt_asset_manager_queue_load(
        am, "../assets/papermill_hdr16f_cube.ktx", (MtAsset **)&skybox_asset);

    mt_asset_manager_queue_load(am, "../assets/helmet_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/boombox_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/lantern_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/sponza_ktx.glb", NULL);

    // Wait for assets to load
    mt_thread_pool_wait_all(&engine->thread_pool);

    mt_environment_set_skybox(&g->scene.env, skybox_asset);

    // Render graph
    mt_render.graph_set_user_data(g->scene.graph, g);
    mt_render.graph_set_builder(g->scene.graph, graph_builder);

    // Create entities
    g->model_archetype = mt_entity_manager_register_archetype(
        em,
        mt_model_archetype_components,
        MT_LENGTH(mt_model_archetype_components),
        mt_model_archetype_init);

    g->light_archetype = mt_entity_manager_register_archetype(
        em,
        mt_light_archetype_components,
        MT_LENGTH(mt_light_archetype_components),
        mt_point_light_archetype_init);

    {
        uint64_t e;
        MtEntityArchetype *arch = g->model_archetype;
        MtModelArchetype *comps = (MtModelArchetype *)arch->components;

        MtPhysicsShape *sphere_shape = NULL;

        MtComponentMask comp_mask = MT_COMP_BIT(MtModelArchetype, transform) |
                                    MT_COMP_BIT(MtModelArchetype, model) |
                                    MT_COMP_BIT(MtModelArchetype, actor);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->transform[e].pos = V3(-1.5, 10, 0);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/helmet_ktx.glb");
        comps->actor[e] = mt_rigid_actor_create(g->scene.physics_scene, MT_RIGID_ACTOR_DYNAMIC);
        sphere_shape = mt_physics_shape_create(g->scene.engine->physics, MT_PHYSICS_SHAPE_SPHERE);
        mt_physics_shape_set_radius(sphere_shape, 1.0f);
        mt_rigid_actor_attach_shape(comps->actor[e], sphere_shape);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/boombox_ktx.glb");
        comps->transform[e].scale = V3(100, 100, 100);
        comps->transform[e].pos = V3(1.5f, 5.f, 0.f);
        comps->actor[e] = mt_rigid_actor_create(g->scene.physics_scene, MT_RIGID_ACTOR_DYNAMIC);
        sphere_shape = mt_physics_shape_create(g->scene.engine->physics, MT_PHYSICS_SHAPE_SPHERE);
        mt_physics_shape_set_radius(sphere_shape, 1.0f);
        mt_rigid_actor_attach_shape(comps->actor[e], sphere_shape);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/lantern_ktx.glb");
        comps->transform[e].scale = V3(0.2f, 0.2f, 0.2f);
        comps->transform[e].pos = V3(4.f, 5.f, 0.f);
        comps->actor[e] = mt_rigid_actor_create(g->scene.physics_scene, MT_RIGID_ACTOR_DYNAMIC);
        sphere_shape = mt_physics_shape_create(g->scene.engine->physics, MT_PHYSICS_SHAPE_SPHERE);
        mt_physics_shape_set_radius(sphere_shape, 1.0f);
        mt_physics_shape_set_local_transform(
            sphere_shape, &(MtPhysicsTransform){.pos = V3(0.0f, 1.1f, 0.0f)});
        mt_rigid_actor_attach_shape(comps->actor[e], sphere_shape);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/sponza_ktx.glb");
        comps->transform[e].pos = V3(0, 0, 0);
        comps->transform[e].scale = V3(3, 3, 3);
        comps->actor[e] = mt_rigid_actor_create(g->scene.physics_scene, MT_RIGID_ACTOR_STATIC);
        MtPhysicsShape *floor_shape =
            mt_physics_shape_create(g->scene.engine->physics, MT_PHYSICS_SHAPE_PLANE);
        mt_physics_shape_set_local_transform(
            floor_shape,
            &(MtPhysicsTransform){.rot = quat_from_axis_angle(V3(0, 0, 1), M_PI / 2.0f)});
        mt_rigid_actor_attach_shape(comps->actor[e], floor_shape);
    }

    MtXorShift xs;
    mt_xor_shift_init(&xs, (uint64_t)time(NULL));

    for (uint32_t i = 0; i < 8; ++i)
    {
        uint64_t e;
        MtEntityArchetype *arch = g->light_archetype;
        MtPointLightArchetype *comps = (MtPointLightArchetype *)arch->components;
        MtComponentMask comp_mask =
            MT_COMP_BIT(MtPointLightArchetype, pos) | MT_COMP_BIT(MtPointLightArchetype, color);

#define LIGHT_POS_X mt_xor_shift_float(&xs, -15.0f, 15.0f)
#define LIGHT_POS_Y mt_xor_shift_float(&xs, 0.0f, 2.0f)
#define LIGHT_POS_Z mt_xor_shift_float(&xs, -10.0f, 10.0f)
#define LIGHT_COL mt_xor_shift_float(&xs, 0.0f, 1.0f)

        e = mt_entity_manager_add_entity(em, g->light_archetype, comp_mask);
        comps->pos[e] = V3(LIGHT_POS_X, LIGHT_POS_Y, LIGHT_POS_Z);
        comps->color[e] = V3(LIGHT_COL, LIGHT_COL, LIGHT_COL);
        comps->color[e] = v3_muls(v3_normalize(comps->color[e]), 10.0f);
    }
}
// }}}

// game_destroy {{{
static void game_destroy(MtScene *inst)
{
    Game *g = (Game *)inst;
    mt_scene_destroy(&g->scene);
}
// }}}

// game_on_event {{{
static void game_on_event(MtScene *inst, const MtEvent *event)
{
    Game *g = (Game *)inst;
    MtEngine *engine = inst->engine;

    switch (event->type)
    {
        case MT_EVENT_BUTTON_PRESSED: {
            if (event->mouse.button == MT_MOUSE_BUTTON_LEFT)
            {
                if (!igIsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !engine->gizmo.hovered)
                {
                    int32_t x, y;
                    mt_window.get_cursor_pos(engine->window, &x, &y);

                    uint32_t value = mt_picker_pick(
                        engine->picker,
                        &inst->cam.uniform,
                        x,
                        y,
                        mt_picking_system,
                        g->model_archetype);

                    g->model_archetype->selected_entity = (MtEntity)value;
                }
            }
            break;
        }
        default: break;
    }
}
// }}}

// game_draw_ui {{{
static void game_draw_ui(MtScene *inst)
{
    Game *g = (Game *)inst;
    MtEngine *engine = inst->engine;

    mt_inspect_archetype(engine, g->model_archetype);
}
// }}}

// game_update {{{
static void game_update(MtScene *inst, float delta)
{
    Game *g = (Game *)inst;

    mt_light_system(g->light_archetype, inst, delta);

    mt_pre_physics_sync_system(g->model_archetype);
    mt_physics_scene_step(inst->physics_scene, delta);
    mt_post_physics_sync_system(g->model_archetype);
}
// }}}

const static MtSceneVT game_vt = {
    .init = game_init,
    .update = game_update,
    .draw_ui = game_draw_ui,
    .on_event = game_on_event,
    .destroy = game_destroy,
};

// game_create {{{
static MtIScene game_create(MtEngine *engine)
{
    Game *g = mt_alloc(engine->alloc, sizeof(Game));
    memset(g, 0, sizeof(*g));
    mt_scene_init(&g->scene, engine);

    return (MtIScene){.vt = &game_vt, .inst = &g->scene};
}
// }}}

int main(int argc, char *argv[])
{
    MtEngine engine = {0};
    mt_engine_init(&engine);

    MtIScene game_scene = game_create(&engine);
    mt_engine_set_scene(&engine, &game_scene);

    while (!mt_window.should_close(engine.window))
    {
        mt_engine_update(&engine);
    }

    mt_engine_destroy(&engine);

    return 0;
}
