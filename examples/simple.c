#include <motor/base/api_types.h>
#include <motor/base/log.h>
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

// Game {{{
typedef struct Game
{
    MtScene scene;

    MtEntityArchetype *model_archetype;
    MtEntityArchetype *light_archetype;
} Game;

static void game_init(Game *g, MtEngine *engine)
{
    memset(g, 0, sizeof(*g));

    mt_scene_init(&g->scene, engine);

    MtEntityManager *em = g->scene.entity_manager;
    MtAssetManager *am = g->scene.asset_manager;

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

        MtPhysicsShape sphere_shape = {.type = MT_PHYSICS_SHAPE_SPHERE, .radius = 1.0f};
        MtPhysicsShape floor_shape = {.type = MT_PHYSICS_SHAPE_PLANE, .plane = V4(0, 1, 0, 0)};

        MtComponentMask comp_mask = MT_COMP_BIT(MtModelArchetype, transform) |
                                    MT_COMP_BIT(MtModelArchetype, model) |
                                    MT_COMP_BIT(MtModelArchetype, actor);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->transform[e].pos = V3(-1.5, 1000, 0);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/helmet_ktx.glb");
        mt_rigid_actor_init(
            g->scene.physics_scene, &comps->actor[e], MT_RIGID_ACTOR_DYNAMIC, &sphere_shape);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/boombox_ktx.glb");
        comps->transform[e].scale = V3(100, 100, 100);
        comps->transform[e].pos = V3(1.5f, 5.f, 0.f);
        mt_rigid_actor_init(
            g->scene.physics_scene, &comps->actor[e], MT_RIGID_ACTOR_DYNAMIC, &sphere_shape);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/lantern_ktx.glb");
        comps->transform[e].scale = V3(0.2f, 0.2f, 0.2f);
        comps->transform[e].pos = V3(4.f, 5.f, 0.f);
        mt_rigid_actor_init(
            g->scene.physics_scene, &comps->actor[e], MT_RIGID_ACTOR_DYNAMIC, &sphere_shape);

        e = mt_entity_manager_add_entity(em, g->model_archetype, comp_mask);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/sponza_ktx.glb");
        comps->transform[e].pos = V3(0, 0, 0);
        comps->transform[e].scale = V3(3, 3, 3);
        mt_rigid_actor_init(
            g->scene.physics_scene, &comps->actor[e], MT_RIGID_ACTOR_STATIC, &floor_shape);
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

static void game_destroy(Game *g)
{
    mt_scene_destroy(&g->scene);
}
// }}}

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

int main(int argc, char *argv[])
{
    MtEngine engine = {0};
    mt_engine_init(&engine);

    Game game = {0};
    game_init(&game, &engine);

    MtWindow *win = engine.window;
    MtSwapchain *swapchain = engine.swapchain;

    mt_render.graph_set_builder(game.scene.graph, graph_builder);

    uint32_t width, height;
    mt_window.get_size(win, &width, &height);

    while (!mt_window.should_close(win))
    {
        mt_file_watcher_poll(engine.watcher, &engine);
        mt_window.poll_events();

        MtEvent event;
        while (mt_window.next_event(win, &event))
        {
            mt_imgui_handle_event(engine.imgui_ctx, &event);
            mt_perspective_camera_on_event(&game.scene.cam, &event);
            switch (event.type)
            {
                case MT_EVENT_WINDOW_CLOSED: {
                    mt_log("Closed");
                    break;
                }
                case MT_EVENT_FRAMEBUFFER_RESIZED: {
                    mt_render.graph_on_resize(game.scene.graph);
                    mt_picker_resize(engine.picker);
                    break;
                }
                case MT_EVENT_BUTTON_PRESSED: {
                    if (event.mouse.button == MT_MOUSE_BUTTON_LEFT)
                    {
                        if (!igIsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
                            !engine.gizmo.hovered)
                        {
                            int32_t x, y;
                            mt_window.get_cursor_pos(win, &x, &y);

                            uint32_t value = mt_picker_pick(
                                engine.picker,
                                &game.scene.cam.uniform,
                                x,
                                y,
                                mt_picking_system,
                                game.model_archetype);

                            game.model_archetype->selected_entity = (MtEntity)value;
                        }
                    }
                    break;
                }
                default: break;
            }
        }

        float delta_time = mt_render.swapchain_get_delta_time(swapchain);

        mt_imgui_begin(engine.imgui_ctx);
        igNewFrame();

        igShowStyleEditor(NULL);

        mt_inspect_archetype(win, game.model_archetype);

        igRender();

        mt_mirror_physics_transforms_system(game.model_archetype);

        // Update cam
        mt_window.get_size(win, &width, &height);
        float aspect = (float)width / (float)height;
        mt_perspective_camera_update(&game.scene.cam, win, aspect, delta_time);

        mt_light_system(game.light_archetype, &game.scene, delta_time);
        mt_physics_scene_step(game.scene.physics_scene, delta_time);

        // Execute render graph
        mt_render.graph_execute(game.scene.graph);
    }

    game_destroy(&game);
    mt_engine_destroy(&engine);

    return 0;
}
