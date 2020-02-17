#include <motor/base/api_types.h>
#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/base/math.h>
#include <motor/base/array.h>
#include <motor/base/rand.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/nuklear.h>
#include <motor/engine/nuklear_impl.h>
#include <motor/engine/file_watcher.h>
#include <motor/engine/engine.h>
#include <motor/engine/camera.h>
#include <motor/engine/environment.h>
#include <motor/engine/entities.h>
#include <motor/engine/entity_archetypes.h>
#include <motor/engine/inspector.h>
#include <motor/engine/physics.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/image_asset.h>
#include <motor/engine/assets/gltf_asset.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Game {{{
typedef struct Game
{
    MtEngine engine;
    MtRenderGraph *graph;
    MtRenderGraph *picking_graph;

    MtImageAsset *image;
    MtPipelineAsset *pbr_pipeline;
    MtPipelineAsset *selected_pipeline;
    MtPipelineAsset *picking_pipeline;
    MtPipelineAsset *fullscreen_pipeline;

    MtPerspectiveCamera cam;
    MtEnvironment env;

    MtEntityArchetype *model_archetype;
    MtEntityArchetype *light_archetype;

    MtPhysicsScene *scene;
} Game;

static void game_init(Game *g)
{
    memset(g, 0, sizeof(*g));

    mt_engine_init(&g->engine);

    MtEntityManager *em = &g->engine.entity_manager;
    MtAssetManager *am = &g->engine.asset_manager;
    MtSwapchain *swapchain = g->engine.swapchain;
    MtDevice *dev = g->engine.device;

    g->scene = mt_physics_scene_create(g->engine.physics);

    MtImageAsset *skybox_asset = NULL;
    mt_asset_manager_queue_load(
        am, "../assets/papermill_hdr16f_cube.ktx", (MtAsset **)&skybox_asset);
    mt_asset_manager_queue_load(am, "../assets/test.png", (MtAsset **)&g->image);
    mt_asset_manager_queue_load(am, "../shaders/pbr.hlsl", (MtAsset **)&g->pbr_pipeline);
    mt_asset_manager_queue_load(am, "../shaders/selected.hlsl", (MtAsset **)&g->selected_pipeline);
    mt_asset_manager_queue_load(am, "../shaders/picking.hlsl", (MtAsset **)&g->picking_pipeline);
    mt_asset_manager_queue_load(
        am, "../shaders/fullscreen.hlsl", (MtAsset **)&g->fullscreen_pipeline);

    mt_asset_manager_queue_load(am, "../assets/helmet_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/boombox_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/lantern_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/sponza_ktx.glb", NULL);

    // Wait for assets to load
    mt_thread_pool_wait_all(&g->engine.thread_pool);

    mt_perspective_camera_init(&g->cam);
    mt_environment_init(&g->env, am, skybox_asset);

    // Create render graph
    g->graph = mt_render.create_graph(dev, swapchain, g);
    g->picking_graph = mt_render.create_graph(dev, NULL, g);

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

        e = mt_entity_manager_add_entity(em, g->model_archetype);
        comps->scale[e] = V3(1, 1, 1);
        comps->pos[e] = V3(-1.5, 1000, 0);
        comps->rot[e] = (Quat){0, 0, 0, 1};
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/helmet_ktx.glb");
        mt_rigid_actor_init(
            g->scene,
            &comps->actor[e],
            MT_RIGID_ACTOR_DYNAMIC,
            &sphere_shape,
            comps->pos[e],
            comps->rot[e]);

        e = mt_entity_manager_add_entity(em, g->model_archetype);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/boombox_ktx.glb");
        comps->scale[e] = V3(100, 100, 100);
        comps->pos[e] = V3(1.5, 5, 0);
        comps->rot[e] = (Quat){0, 0, 0, 1};
        mt_rigid_actor_init(
            g->scene,
            &comps->actor[e],
            MT_RIGID_ACTOR_DYNAMIC,
            &sphere_shape,
            comps->pos[e],
            comps->rot[e]);

        e = mt_entity_manager_add_entity(em, g->model_archetype);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/lantern_ktx.glb");
        comps->scale[e] = V3(0.2f, 0.2f, 0.2f);
        comps->pos[e] = V3(4, 5, 0);
        comps->rot[e] = (Quat){0, 0, 0, 1};
        mt_rigid_actor_init(
            g->scene,
            &comps->actor[e],
            MT_RIGID_ACTOR_DYNAMIC,
            &sphere_shape,
            comps->pos[e],
            comps->rot[e]);

        e = mt_entity_manager_add_entity(em, g->model_archetype);
        comps->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/sponza_ktx.glb");
        comps->pos[e] = V3(0, 0, 0);
        comps->scale[e] = V3(3, 3, 3);
        comps->rot[e] = (Quat){0, 0, 0, 1};
        mt_rigid_actor_init(
            g->scene,
            &comps->actor[e],
            MT_RIGID_ACTOR_STATIC,
            &floor_shape,
            comps->pos[e],
            comps->rot[e]);
    }

    MtXorShift xs;
    mt_xor_shift_init(&xs, (uint64_t)time(NULL));

    for (uint32_t i = 0; i < 8; ++i)
    {
        uint64_t e;
        MtEntityArchetype *arch = g->light_archetype;
        MtPointLightArchetype *comps = (MtPointLightArchetype *)arch->components;

#define LIGHT_POS_X mt_xor_shift_float(&xs, -15.0f, 15.0f)
#define LIGHT_POS_Y mt_xor_shift_float(&xs, 0.0f, 2.0f)
#define LIGHT_POS_Z mt_xor_shift_float(&xs, -10.0f, 10.0f)
#define LIGHT_COL mt_xor_shift_float(&xs, 0.0f, 1.0f)

        e = mt_entity_manager_add_entity(em, g->light_archetype);
        comps->pos[e] = V3(LIGHT_POS_X, LIGHT_POS_Y, LIGHT_POS_Z);
        comps->color[e] = V3(LIGHT_COL, LIGHT_COL, LIGHT_COL);
        comps->color[e] = v3_muls(v3_normalize(comps->color[e]), 10.0f);
    }
}

static void game_destroy(Game *g)
{
    mt_physics_scene_destroy(g->scene);
    mt_render.destroy_graph(g->picking_graph);
    mt_render.destroy_graph(g->graph);
    mt_environment_destroy(&g->env);
    mt_engine_destroy(&g->engine);
}
// }}}

// Light system {{{
static void light_system(MtEntityArchetype *arch, MtEnvironment *env, float delta)
{
    MtPointLightArchetype *comps = (MtPointLightArchetype *)arch->components;

    static float acc = 0.0f;
    acc += delta;

    float x = sinf(acc * 2.0f) * 2.0f;
    float z = cosf(acc * 2.0f) * 2.0f;

    const float constant = 1.0f;
    const float linear = 0.7f;
    const float quadratic = 1.8f;
    float light_max = 10.0f;
    float radius =
        (-linear +
         sqrtf(linear * linear - 4.0f * quadratic * (constant - (256.0f / 5.0f) * light_max))) /
        (2 * quadratic);

    env->uniform.point_light_count = 0;
    for (uint32_t e = 0; e < arch->entity_count; ++e)
    {
        uint32_t l = env->uniform.point_light_count;

        env->uniform.point_lights[l].pos.xyz = comps->pos[e];
        env->uniform.point_lights[l].pos.x += x;
        env->uniform.point_lights[l].pos.z += z;
        env->uniform.point_lights[l].pos.w = 1.0f;

        env->uniform.point_lights[l].color = comps->color[e];

        env->uniform.point_lights[l].radius = radius;

        env->uniform.point_light_count++;
    }
}
// }}}

// Model system {{{
static void model_system(MtCmdBuffer *cb, Game *g, MtEntityArchetype *arch)
{
    mt_render.cmd_bind_pipeline(cb, g->pbr_pipeline->pipeline);
    mt_render.cmd_bind_uniform(cb, &g->cam.uniform, sizeof(g->cam.uniform), 0, 0);
    mt_environment_bind(&g->env, cb, 3);

    MtModelArchetype *comps = (MtModelArchetype *)arch->components;
    for (MtEntity e = 0; e < arch->entity_count; ++e)
    {
        mt_rigid_actor_get_transform(&comps->actor[e], &comps->pos[e], &comps->rot[e]);

        Mat4 transform = mat4_identity();
        transform = mat4_scale(transform, comps->scale[e]);
        transform = mat4_mul(quat_to_mat4(comps->rot[e]), transform);
        transform = mat4_translate(transform, comps->pos[e]);

        mt_gltf_asset_draw(comps->model[e], cb, &transform, 1, 2);
    }

    if (arch->selected_entity != MT_ENTITY_INVALID)
    {
        MtEntity e = arch->selected_entity;

        Mat4 transform = mat4_identity();
        transform = mat4_scale(transform, comps->scale[e]);
        transform = mat4_mul(quat_to_mat4(comps->rot[e]), transform);
        transform = mat4_translate(transform, comps->pos[e]);

        // Draw wireframe
        mt_render.cmd_bind_pipeline(cb, g->selected_pipeline->pipeline);
        mt_render.cmd_bind_uniform(cb, &g->cam.uniform, sizeof(g->cam.uniform), 0, 0);
        mt_gltf_asset_draw(comps->model[e], cb, &transform, 1, UINT32_MAX);
    }
}
// }}}

// Physics transform mirroring {{{
static void mirror_physics_transforms_system(MtEntityArchetype *arch)
{
    MtModelArchetype *comps = (MtModelArchetype *)arch->components;
    for (MtEntity e = 0; e < arch->entity_count; ++e)
    {
        mt_rigid_actor_set_transform(&comps->actor[e], comps->pos[e], comps->rot[e]);
    }
}
// }}}

// Picking system {{{
static void picking_system(MtCmdBuffer *cb, MtEntityArchetype *arch)
{
    MtModelArchetype *comps = (MtModelArchetype *)arch->components;
    for (uint32_t e = 0; e < arch->entity_count; ++e)
    {
        Mat4 transform = mat4_identity();
        transform = mat4_scale(transform, comps->scale[e]);
        transform = mat4_mul(quat_to_mat4(comps->rot[e]), transform);
        transform = mat4_translate(transform, comps->pos[e]);

        const uint32_t entity_id = e;
        mt_render.cmd_bind_uniform(cb, &entity_id, sizeof(uint32_t), 2, 0);
        mt_gltf_asset_draw(comps->model[e], cb, &transform, 1, UINT32_MAX);
    }
}
// }}}

// Picking render graph {{{
static void picking_pass_builder(MtRenderGraph *graph, MtCmdBuffer *cb, void *user_data)
{
    Game *g = user_data;

    // Draw models
    mt_render.cmd_bind_pipeline(cb, g->picking_pipeline->pipeline);
    mt_render.cmd_bind_uniform(cb, &g->cam.uniform, sizeof(g->cam.uniform), 0, 0);
    picking_system(cb, g->model_archetype);
}

static void picking_transfer_pass_builder(MtRenderGraph *graph, MtCmdBuffer *cb, void *user_data)
{
    Game *g = user_data;

    double x, y;
    mt_window.get_cursor_pos(g->engine.window, &x, &y);

    struct nk_context *nk = mt_nuklear_get_context(g->engine.nk_ctx);

    MtBuffer *picking_buffer = mt_render.graph_get_buffer(g->picking_graph, "picking_buffer");

    if (x > 0.0 && y > 0.0 && !nk_window_is_any_hovered(nk))
    {
        mt_render.cmd_copy_image_to_buffer(
            cb,
            &(MtImageCopyView){
                .image = mt_render.graph_get_image(graph, "picking"),
                .offset = {(int32_t)x, (int32_t)y, 0},
            },
            &(MtBufferCopyView){.buffer = picking_buffer},
            (MtExtent3D){1, 1, 1});
    }
}

static void picking_graph_builder(MtRenderGraph *graph, void *user_data)
{
    Game *g = user_data;

    uint32_t width, height;
    mt_window.get_size(g->engine.window, &width, &height);

    mt_render.graph_add_image(
        graph,
        "depth",
        &(MtImageCreateInfo){
            .width = width,
            .height = height,
            .format = MT_FORMAT_D32_SFLOAT,
        });

    mt_render.graph_add_image(
        graph,
        "picking",
        &(MtImageCreateInfo){
            .width = width,
            .height = height,
            .format = MT_FORMAT_R32_UINT,
        });

    mt_render.graph_add_buffer(
        graph,
        "picking_buffer",
        &(MtBufferCreateInfo){
            .usage = MT_BUFFER_USAGE_TRANSFER,
            .memory = MT_BUFFER_MEMORY_HOST,
            .size = sizeof(uint32_t),
        });

    {
        MtRenderGraphPass *picking_pass =
            mt_render.graph_add_pass(graph, "picking_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
        mt_render.pass_write(picking_pass, MT_PASS_WRITE_COLOR_ATTACHMENT, "picking");
        mt_render.pass_write(picking_pass, MT_PASS_WRITE_DEPTH_STENCIL_ATTACHMENT, "depth");
        mt_render.pass_set_builder(picking_pass, picking_pass_builder);
    }

    {
        MtRenderGraphPass *picking_transfer_pass =
            mt_render.graph_add_pass(graph, "picking_transfer_pass", MT_PIPELINE_STAGE_TRANSFER);
        mt_render.pass_read(picking_transfer_pass, MT_PASS_READ_IMAGE_TRANSFER, "picking");
        mt_render.pass_write(picking_transfer_pass, MT_PASS_WRITE_BUFFER, "picking_buffer");
        mt_render.pass_set_builder(picking_transfer_pass, picking_transfer_pass_builder);
    }
}
// }}}

// Color render graph {{{
static void color_pass_builder(MtRenderGraph *graph, MtCmdBuffer *cb, void *user_data)
{
    Game *g = user_data;
    MtNuklearContext *nk_ctx = g->engine.nk_ctx;

    // Draw skybox
    mt_render.cmd_bind_uniform(cb, &g->cam.uniform, sizeof(g->cam.uniform), 0, 0);
    mt_environment_draw_skybox(&g->env, cb);

    // Draw models
    model_system(cb, g, g->model_archetype);

    // Draw UI
    mt_nuklear_render(nk_ctx, cb);
}

static void graph_builder(MtRenderGraph *graph, void *user_data)
{
    Game *g = user_data;

    uint32_t width, height;
    mt_window.get_size(g->engine.window, &width, &height);

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
    Game game = {0};
    game_init(&game);

    MtWindow *win = game.engine.window;
    MtSwapchain *swapchain = game.engine.swapchain;
    MtDevice *dev = game.engine.device;
    struct nk_context *nk = mt_nuklear_get_context(game.engine.nk_ctx);

    mt_render.graph_set_builder(game.graph, graph_builder);
    mt_render.graph_bake(game.graph);

    mt_render.graph_set_builder(game.picking_graph, picking_graph_builder);
    mt_render.graph_bake(game.picking_graph);

    uint32_t width, height;
    mt_window.get_size(win, &width, &height);

    while (!mt_window.should_close(win))
    {
        mt_file_watcher_poll(game.engine.watcher, &game.engine);
        mt_window.poll_events();

        nk_input_begin(nk);

        MtEvent event;
        while (mt_window.next_event(win, &event))
        {
            mt_nuklear_handle_event(game.engine.nk_ctx, &event);
            mt_perspective_camera_on_event(&game.cam, &event);
            switch (event.type)
            {
                case MT_EVENT_WINDOW_CLOSED: {
                    mt_log("Closed");
                    break;
                }
                case MT_EVENT_FRAMEBUFFER_RESIZED: {
                    mt_render.graph_on_resize(game.graph);
                    mt_render.graph_on_resize(game.picking_graph);
                    break;
                }
                case MT_EVENT_BUTTON_PRESSED: {
                    if (event.mouse.button == MT_MOUSE_BUTTON_LEFT)
                    {
                        mt_render.graph_execute(game.picking_graph);
                        mt_render.graph_wait_all(game.picking_graph);

                        MtBuffer *picking_buffer =
                            mt_render.graph_get_buffer(game.picking_graph, "picking_buffer");

                        void *mapping = mt_render.map_buffer(dev, picking_buffer);
                        MtEntity entity_id = 0;
                        memcpy(&entity_id, mapping, sizeof(uint32_t));

                        if (entity_id > 0)
                        {
                            game.model_archetype->selected_entity = entity_id - 1;
                        }

                        memcpy(mapping, &(uint32_t){0}, sizeof(uint32_t));
                        mt_render.unmap_buffer(dev, picking_buffer);
                    }
                    break;
                }
                default: break;
            }
        }

        nk_input_end(nk);

        float delta_time = mt_render.swapchain_get_delta_time(swapchain);

        // UI
        if (nk_begin(
                nk,
                "Demo",
                nk_rect(50, 50, 230, 250),
                NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE |
                    NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(nk, 0, 2);

            nk_labelf(nk, NK_TEXT_ALIGN_LEFT, "Delta");
            nk_labelf(nk, NK_TEXT_ALIGN_LEFT, "%fms", delta_time);

            nk_labelf(nk, NK_TEXT_ALIGN_LEFT, "FPS");
            nk_labelf(nk, NK_TEXT_ALIGN_LEFT, "%.0f", 1.0f / delta_time);

            nk_labelf(nk, NK_TEXT_ALIGN_LEFT, "Camera pos");
            nk_labelf(
                nk,
                NK_TEXT_ALIGN_LEFT,
                "%.2f %.2f %.2f",
                game.cam.pos.x,
                game.cam.pos.y,
                game.cam.pos.z);

            nk_layout_row_dynamic(nk, 32, 1);
            if (nk_button_label(nk, "Hello"))
            {
                mt_log("Hello");
            }
        }
        nk_end(nk);

        mt_inspect_archetype(win, nk, game.model_archetype);

        mirror_physics_transforms_system(game.model_archetype);

        // Update cam
        mt_window.get_size(win, &width, &height);
        float aspect = (float)width / (float)height;
        mt_perspective_camera_update(&game.cam, win, aspect, delta_time);

        light_system(game.light_archetype, &game.env, delta_time);
        mt_physics_scene_step(game.scene, delta_time);

        // Execute render graph
        mt_render.graph_execute(game.graph);
    }

    game_destroy(&game);

    return 0;
}
