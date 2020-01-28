#include <motor/base/allocator.h>
#include <motor/base/util.h>
#include <motor/base/math.h>
#include <motor/base/array.h>
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
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct Game
{
    MtEngine engine;

    MtImageAsset *image;
    MtPipelineAsset *model_pipeline;

    MtPerspectiveCamera cam;
    MtEnvironment env;
} Game;

static void game_init(Game *g)
{
    memset(g, 0, sizeof(*g));

    mt_engine_init(&g->engine);

    MtImageAsset *skybox_asset = NULL;
    mt_asset_manager_queue_load(
        &g->engine.asset_manager, "../assets/papermill_hdr16f_cube.ktx", (MtAsset **)&skybox_asset);
    mt_asset_manager_queue_load(
        &g->engine.asset_manager, "../assets/test.png", (MtAsset **)&g->image);
    mt_asset_manager_queue_load(
        &g->engine.asset_manager, "../assets/shaders/pbr.glsl", (MtAsset **)&g->model_pipeline);

    mt_asset_manager_queue_load(&g->engine.asset_manager, "../assets/helmet_ktx.glb", NULL);
    mt_asset_manager_queue_load(&g->engine.asset_manager, "../assets/boombox_ktx.glb", NULL);

    // Wait for assets to load
    mt_thread_pool_wait_all(&g->engine.thread_pool);

    mt_perspective_camera_init(&g->cam);
    mt_environment_init(&g->env, &g->engine.asset_manager, skybox_asset);
}

static void game_destroy(Game *g)
{
    mt_environment_destroy(&g->env);
    mt_engine_destroy(&g->engine);
}

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
            transform      = mat4_scale(transform, b->scale[i]);
            transform      = mat4_mul(quat_to_mat4(b->rot[i]), transform);
            transform      = mat4_translate(transform, b->pos[i]);

            mt_gltf_asset_draw(b->model[i], cb, &transform, 1, 3);
        }
    }
}

static void draw_ui(Game *g)
{
    MtSwapchain *swapchain = g->engine.swapchain;
    MtUIRenderer *ui       = g->engine.ui;

    float delta_time = mt_render.swapchain_get_delta_time(swapchain);
    mt_ui_printf(ui, "Delta: %fms", delta_time);

    mt_ui_printf(
        ui,
        "Pos: %.2f  %.2f  %.2f",
        g->cam.uniform.pos.x,
        g->cam.uniform.pos.y,
        g->cam.uniform.pos.z);

    mt_ui_image(ui, g->image->image, 64, 64);

    mt_ui_set_color(ui, V3(1, 0, 0));
    mt_ui_rect(ui, 64, 64);
    mt_ui_set_color(ui, V3(1, 1, 1));

    mt_ui_rect(ui, 64, 64);
    if (mt_ui_button(ui, "Hello"))
    {
        printf("Hello\n");
    }

    if (mt_ui_button(ui, "Hello 2"))
    {
        printf("Hello 2\n");
    }
}

int main(int argc, char *argv[])
{
    Game game = {0};
    game_init(&game);

    MtWindow *win          = game.engine.window;
    MtSwapchain *swapchain = game.engine.swapchain;
    MtEntityManager *em = &game.engine.entity_manager;
    MtAssetManager *am  = &game.engine.asset_manager;
    MtUIRenderer *ui = game.engine.ui;

    uint32_t model_archetype =
        mt_entity_manager_register_archetype(em, mt_model_archetype_init, sizeof(MtModelArchetype));

    MtModelArchetype *block;
    uint32_t e;

    {
        block           = mt_entity_manager_add_entity(em, model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/helmet_ktx.glb");
        block->pos[e]   = V3(-1.5, 0, 0);
    }

    {
        block           = mt_entity_manager_add_entity(em, model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/boombox_ktx.glb");
        block->scale[e] = V3(100, 100, 100);
        block->pos[e]   = V3(1.5, 0, 0);
    }

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
                case MT_EVENT_WINDOW_CLOSED:
                {
                    printf("Closed\n");
                    break;
                }
                default: break;
            }
        }

        MtCmdBuffer *cb = mt_render.swapchain_begin_frame(swapchain);
        mt_render.begin_cmd_buffer(cb);
        mt_render.cmd_begin_render_pass(
            cb, mt_render.swapchain_get_render_pass(swapchain), NULL, NULL);

        // Update cam
        uint32_t width, height;
        mt_window.get_size(win, &width, &height);
        float aspect     = (float)width / (float)height;
        float delta_time = mt_render.swapchain_get_delta_time(swapchain);
        mt_perspective_camera_update(&game.cam, win, aspect, delta_time);

        // Begin UI
        MtViewport viewport;
        mt_render.cmd_get_viewport(cb, &viewport);
        mt_ui_begin(ui, &viewport);

        // Submit UI commands
        draw_ui(&game);

        // Draw skybox
        mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);
        mt_environment_draw_skybox(&game.env, cb);

        // Draw model
        mt_render.cmd_bind_pipeline(cb, game.model_pipeline->pipeline);
        mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);
        mt_environment_bind(&game.env, cb, 2);
        model_system(cb, &game.engine.entity_manager.archetypes[model_archetype]);

        // Draw UI
        mt_ui_draw(ui, cb);

        mt_render.cmd_end_render_pass(cb);
        mt_render.end_cmd_buffer(cb);
        mt_render.swapchain_end_frame(swapchain);
    }

    game_destroy(&game);

    return 0;
}
