#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/base/util.h>
#include <motor/base/math.h>
#include <motor/base/array.h>
#include <motor/base/rand.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/ui.h>
#include <motor/engine/file_watcher.h>
#include <motor/engine/engine.h>
#include <motor/engine/camera.h>
#include <motor/engine/environment.h>
#include <motor/engine/entities.h>
#include <motor/engine/entity_archetypes.h>
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

    MtImageAsset *image;
    MtPipelineAsset *pbr_pipeline;
    MtPipelineAsset *fullscreen_pipeline;

    MtPerspectiveCamera cam;
    MtEnvironment env;

    uint32_t model_archetype;
    uint32_t light_archetype;
} Game;

static void game_init(Game *g)
{
    memset(g, 0, sizeof(*g));

    mt_engine_init(&g->engine);

    MtEntityManager *em = &g->engine.entity_manager;
    MtAssetManager *am = &g->engine.asset_manager;
    MtSwapchain *swapchain = g->engine.swapchain;
    MtDevice *dev = g->engine.device;

    MtImageAsset *skybox_asset = NULL;
    mt_asset_manager_queue_load(
        am, "../assets/papermill_hdr16f_cube.ktx", (MtAsset **)&skybox_asset);
    mt_asset_manager_queue_load(am, "../assets/test.png", (MtAsset **)&g->image);
    mt_asset_manager_queue_load(am, "../assets/shaders/pbr.hlsl", (MtAsset **)&g->pbr_pipeline);
    mt_asset_manager_queue_load(
        am, "../assets/shaders/fullscreen.hlsl", (MtAsset **)&g->fullscreen_pipeline);

    mt_asset_manager_queue_load(am, "../assets/helmet_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/boombox_ktx.glb", NULL);
    mt_asset_manager_queue_load(am, "../assets/sponza_ktx.glb", NULL);

    // Wait for assets to load
    mt_thread_pool_wait_all(&g->engine.thread_pool);

    mt_perspective_camera_init(&g->cam);
    mt_environment_init(&g->env, am, skybox_asset);

    // Create render graph
    g->graph = mt_render.create_graph(dev, swapchain, g);

    // Create entities
    g->model_archetype =
        mt_entity_manager_register_archetype(em, mt_model_archetype_init, sizeof(MtModelArchetype));

    g->light_archetype = mt_entity_manager_register_archetype(
        em, mt_point_light_archetype_init, sizeof(MtPointLightArchetype));

    {
        MtModelArchetype *block;
        uint32_t e;

        block = mt_entity_manager_add_entity(em, g->model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/helmet_ktx.glb");
        block->pos[e] = V3(-1.5, 1, 0);

        block = mt_entity_manager_add_entity(em, g->model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/boombox_ktx.glb");
        block->scale[e] = V3(100, 100, 100);
        block->pos[e] = V3(1.5, 1, 0);

        block = mt_entity_manager_add_entity(em, g->model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/sponza_ktx.glb");
        block->scale[e] = V3(3, 3, 3);
    }

    MtXorShift xs;
    mt_xor_shift_init(&xs, (uint64_t)time(NULL));

    for (uint32_t i = 0; i < 64; ++i)
    {
        MtPointLightArchetype *block;
        uint32_t e;

#define LIGHT_POS_X mt_xor_shift_float(&xs, -30.0f, 30.0f)
#define LIGHT_POS_Y mt_xor_shift_float(&xs, 0.0f, 2.0f)
#define LIGHT_POS_Z mt_xor_shift_float(&xs, -20.0f, 20.0f)
#define LIGHT_COL mt_xor_shift_float(&xs, 0.0f, 1.0f)

        block = mt_entity_manager_add_entity(em, g->light_archetype, &e);
        block->pos[e] = V3(LIGHT_POS_X, LIGHT_POS_Y, LIGHT_POS_Z);
        block->color[e] = V3(LIGHT_COL, LIGHT_COL, LIGHT_COL);
        block->color[e] = v3_muls(v3_normalize(block->color[e]), 10.0f);
    }
}

static void game_destroy(Game *g)
{
    mt_render.destroy_graph(g->graph);
    mt_environment_destroy(&g->env);
    mt_engine_destroy(&g->engine);
}
// }}}

// Light system {{{
static void light_system(MtEntityArchetype *archetype, MtEnvironment *env, float delta)
{
    static float acc = 0.0f;
    acc += delta;

    float x = sin(acc * 2.0f) * 2.0f;
    float z = cos(acc * 2.0f) * 2.0f;

    const float constant = 1.0;
    const float linear = 0.7;
    const float quadratic = 1.8;
    float light_max = 10.0f;
    float radius =
        (-linear +
         sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * light_max))) /
        (2 * quadratic);

    env->uniform.point_light_count = 0;
    for (MtEntityBlock *block = archetype->blocks;
         block != (archetype->blocks + mt_array_size(archetype->blocks));
         ++block)
    {
        for (uint32_t i = 0; i < block->entity_count; ++i)
        {
            uint32_t l = env->uniform.point_light_count;

            MtPointLightArchetype *b = block->data;
            env->uniform.point_lights[l].pos.xyz = b->pos[i];
            env->uniform.point_lights[l].pos.x += x;
            env->uniform.point_lights[l].pos.z += z;
            env->uniform.point_lights[l].pos.w = 1.0f;

            env->uniform.point_lights[l].color = b->color[i];

            env->uniform.point_lights[l].radius = radius;

            env->uniform.point_light_count++;
        }
    }
}
// }}}

// Model system {{{
static void model_system(MtCmdBuffer *cb, MtEntityArchetype *archetype)
{
    for (MtEntityBlock *block = archetype->blocks;
         block != (archetype->blocks + mt_array_size(archetype->blocks));
         ++block)
    {
        for (uint32_t i = 0; i < block->entity_count; ++i)
        {
            MtModelArchetype *b = block->data;

            Mat4 transform = mat4_identity();
            transform = mat4_scale(transform, b->scale[i]);
            transform = mat4_mul(quat_to_mat4(b->rot[i]), transform);
            transform = mat4_translate(transform, b->pos[i]);

            mt_gltf_asset_draw(b->model[i], cb, &transform, 1, 2);
        }
    }
}
// }}}

// UI {{{
static void draw_ui(Game *g)
{
    MtSwapchain *swapchain = g->engine.swapchain;
    MtUIRenderer *ui = g->engine.ui;

    float delta_time = mt_render.swapchain_get_delta_time(swapchain);
    mt_ui_printf(ui, "Delta: %fms", delta_time);
    mt_ui_printf(ui, "FPS: %.0f", 1.0f / delta_time);

    mt_ui_printf(
        ui,
        "Pos: %.2f  %.2f  %.2f",
        g->cam.uniform.pos.x,
        g->cam.uniform.pos.y,
        g->cam.uniform.pos.z);

    mt_ui_image(ui, g->image->image, 64, 64);
}
// }}}

static void color_pass_builder(MtRenderGraph *graph, MtCmdBuffer *cb, void *user_data)
{
    Game *g = user_data;
    MtEntityManager *em = &g->engine.entity_manager;

    // Draw skybox
    mt_render.cmd_bind_uniform(cb, &g->cam.uniform, sizeof(g->cam.uniform), 0, 0);
    mt_environment_draw_skybox(&g->env, cb);

    // Draw models
    mt_render.cmd_bind_pipeline(cb, g->pbr_pipeline->pipeline);
    mt_render.cmd_bind_uniform(cb, &g->cam.uniform, sizeof(g->cam.uniform), 0, 0);
    mt_environment_bind(&g->env, cb, 3);
    model_system(cb, &em->archetypes[g->model_archetype]);

    // Begin UI
    MtUIRenderer *ui = g->engine.ui;

    MtViewport viewport;
    mt_render.cmd_get_viewport(cb, &viewport);
    mt_ui_begin(ui, &viewport);

    // Submit UI commands
    draw_ui(g);

    // Draw UI
    mt_ui_draw(ui, cb);
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

    MtRenderGraphPass *color_pass =
        mt_render.graph_add_pass(graph, "color_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
    mt_render.pass_write(color_pass, MT_PASS_WRITE_DEPTH_STENCIL_ATTACHMENT, "depth");
    mt_render.pass_set_builder(color_pass, color_pass_builder);
}

int main(int argc, char *argv[])
{
    Game game = {0};
    game_init(&game);

    MtWindow *win = game.engine.window;
    MtSwapchain *swapchain = game.engine.swapchain;
    MtEntityManager *em = &game.engine.entity_manager;
    MtUIRenderer *ui = game.engine.ui;

    mt_render.graph_set_builder(game.graph, graph_builder);
    mt_render.graph_bake(game.graph);

    uint32_t width, height;
    mt_window.get_size(win, &width, &height);

    while (!mt_window.should_close(win))
    {
        mt_file_watcher_poll(game.engine.watcher, &game.engine);
        mt_window.poll_events();

        MtEvent event;
        while (mt_window.next_event(win, &event))
        {
            mt_ui_on_event(ui, &event);
            mt_perspective_camera_on_event(&game.cam, &event);
            switch (event.type)
            {
                case MT_EVENT_FRAMEBUFFER_RESIZED:
                {
                    break;
                }
                case MT_EVENT_WINDOW_CLOSED:
                {
                    mt_log("Closed");
                    break;
                }
                default: break;
            }
        }

        // Update cam
        mt_window.get_size(win, &width, &height);
        float aspect = (float)width / (float)height;
        float delta_time = mt_render.swapchain_get_delta_time(swapchain);
        mt_perspective_camera_update(&game.cam, win, aspect, delta_time);

        light_system(&em->archetypes[game.light_archetype], &game.env, delta_time);

        // Execute render graph
        mt_render.graph_execute(game.graph);
    }

    game_destroy(&game);

    return 0;
}
