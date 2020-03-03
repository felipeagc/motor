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
#include <motor/engine/components.h>
#include <motor/engine/imgui_impl.h>
#include <motor/engine/inspector.h>
#include <motor/engine/picker.h>
#include <motor/engine/systems.h>
#include <motor/engine/physics.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct Game
{
    /*base*/ MtScene scene;
} Game;

// game_init {{{
static void game_init(MtScene *scene)
{
    Game *g = (Game *)scene;

    MtEngine *engine = scene->engine;
    MtEntityManager *em = scene->entity_manager;
    MtAssetManager *am = scene->asset_manager;

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

    // Build render graph
    {
        MtRenderGraphImage depth_info = {
            .size_class = MT_SIZE_CLASS_SWAPCHAIN_RELATIVE,
            .width = 1.0f,
            .height = 1.0f,
            .format = MT_FORMAT_D32_SFLOAT,
        };

        mt_render.graph_add_image(scene->graph, "depth", &depth_info);

        {
            MtRenderGraphPass *color_pass = mt_render.graph_add_pass(
                scene->graph, "color_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
            mt_render.pass_write(color_pass, MT_PASS_WRITE_DEPTH_STENCIL_ATTACHMENT, "depth");
        }
    }

    // Create entities
    {
        uint64_t e;
        MtDefaultComponents *comps = (MtDefaultComponents *)em->components;

        MtPhysicsShape *sphere_shape = NULL;

        MtComponentMask comp_mask = MT_COMP_BIT(MtDefaultComponents, transform) |
                                    MT_COMP_BIT(MtDefaultComponents, model) |
                                    MT_COMP_BIT(MtDefaultComponents, actor);

        e = mt_entity_manager_add_entity(em, comp_mask);
        comps->transform[e].pos = V3(-1.5, 10, 0);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/helmet_ktx.glb");
        comps->actor[e] = mt_rigid_actor_create(g->scene.physics_scene, MT_RIGID_ACTOR_DYNAMIC);
        sphere_shape = mt_physics_shape_create(g->scene.engine->physics, MT_PHYSICS_SHAPE_SPHERE);
        mt_physics_shape_set_radius(sphere_shape, 1.0f);
        mt_rigid_actor_attach_shape(comps->actor[e], sphere_shape);

        e = mt_entity_manager_add_entity(em, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/boombox_ktx.glb");
        comps->transform[e].scale = V3(100, 100, 100);
        comps->transform[e].pos = V3(1.5f, 5.f, 0.f);
        comps->actor[e] = mt_rigid_actor_create(g->scene.physics_scene, MT_RIGID_ACTOR_DYNAMIC);
        sphere_shape = mt_physics_shape_create(g->scene.engine->physics, MT_PHYSICS_SHAPE_SPHERE);
        mt_physics_shape_set_radius(sphere_shape, 1.0f);
        mt_rigid_actor_attach_shape(comps->actor[e], sphere_shape);

        e = mt_entity_manager_add_entity(em, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/lantern_ktx.glb");
        comps->transform[e].scale = V3(0.2f, 0.2f, 0.2f);
        comps->transform[e].pos = V3(4.f, 5.f, 0.f);
        comps->actor[e] = mt_rigid_actor_create(g->scene.physics_scene, MT_RIGID_ACTOR_DYNAMIC);
        sphere_shape = mt_physics_shape_create(g->scene.engine->physics, MT_PHYSICS_SHAPE_SPHERE);
        mt_physics_shape_set_radius(sphere_shape, 1.0f);
        mt_physics_shape_set_local_transform(
            sphere_shape,
            &(MtPhysicsTransform){.pos = V3(0.0f, 1.1f, 0.0f), .rot = (Quat){0, 0, 0, 1}});
        mt_rigid_actor_attach_shape(comps->actor[e], sphere_shape);

        e = mt_entity_manager_add_entity(em, comp_mask);
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
        MtDefaultComponents *comps = (MtDefaultComponents *)em->components;
        MtComponentMask comp_mask = MT_COMP_BIT(MtDefaultComponents, transform) |
                                    MT_COMP_BIT(MtDefaultComponents, point_light);

#define LIGHT_POS_X mt_xor_shift_float(&xs, -15.0f, 15.0f)
#define LIGHT_POS_Y mt_xor_shift_float(&xs, 0.0f, 2.0f)
#define LIGHT_POS_Z mt_xor_shift_float(&xs, -10.0f, 10.0f)
#define LIGHT_COL mt_xor_shift_float(&xs, 0.0f, 1.0f)

        e = mt_entity_manager_add_entity(em, comp_mask);
        comps->transform[e].pos = V3(LIGHT_POS_X, LIGHT_POS_Y, LIGHT_POS_Z);
        comps->point_light[e].color = V3(LIGHT_COL, LIGHT_COL, LIGHT_COL);
    }
}
// }}}

// game_destroy {{{
static void game_destroy(MtScene *scene)
{
    Game *g = (Game *)scene;
    mt_scene_destroy(&g->scene);
}
// }}}

// game_on_event {{{
static void game_on_event(MtScene *scene, const MtEvent *event)
{
    MtEngine *engine = scene->engine;
    MtEntityManager *em = scene->entity_manager;

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
                        engine->picker, &scene->cam.uniform, x, y, mt_picking_system, em);

                    em->selected_entity = (MtEntity)value;
                }
            }
            break;
        }
        default: break;
    }
}
// }}}

// game_draw_ui {{{
static void game_draw_ui(MtScene *scene)
{
    MtEngine *engine = scene->engine;
    MtEntityManager *em = scene->entity_manager;

    mt_inspect_entities(engine, em);
}
// }}}

// game_update {{{
static void game_update(MtScene *scene, float delta)
{
    Game *g = (Game *)scene;
    MtEntityManager *em = scene->entity_manager;

    mt_light_system(em, scene, delta);

    mt_pre_physics_sync_system(em);
    mt_physics_scene_step(scene->physics_scene, delta);
    mt_post_physics_sync_system(em);

    {
        MtCmdBuffer *cb = mt_render.pass_begin(scene->graph, "color_pass");

        // Draw skybox
        mt_environment_draw_skybox(&g->scene.env, cb, &g->scene.cam.uniform);

        // Draw models
        mt_model_system(em, &g->scene, cb);
        mt_selected_entity_system(em, &g->scene, cb);

        // Draw UI
        mt_imgui_render(g->scene.engine->imgui_ctx, cb);

        mt_render.pass_end(scene->graph, "color_pass");
    }
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
