#include <motor/base/api_types.h>
#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/base/math.h>
#include <motor/base/rand.h>
#include <motor/base/buffer_writer.h>
#include <motor/base/buffer_reader.h>
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
static void game_draw_ui(MtScene *scene, float delta)
{
    MtEngine *engine = scene->engine;
    MtEntityManager *em = scene->entity_manager;

    mt_inspect_entities(engine, em);

    if (igBegin("Info", NULL, 0))
    {
        igText("Delta: %.4f ms", delta);
        igText("FPS  : %.0f", 1.0f / delta);

        if (igButton("Save", (ImVec2){}))
        {
            MtBufferWriter bw;
            mt_buffer_writer_init(&bw, engine->alloc);

            mt_entity_manager_serialize(em, &bw);

            size_t size = 0;
            uint8_t *buf = mt_buffer_writer_build(&bw, &size);

            FILE *f = fopen("../assets/scene.bin", "wb");
            if (f)
            {
                fwrite(buf, 1, size, f);
                fclose(f);
            }

            mt_buffer_writer_destroy(&bw);
        }

        if (igButton("Load", (ImVec2){}))
        {
            FILE *f = fopen("../assets/scene.bin", "rb");
            size_t size = 0;
            uint8_t *buf = NULL;
            if (f)
            {
                fseek(f, 0, SEEK_END);
                size = ftell(f);
                fseek(f, 0, SEEK_SET);
                buf = mt_alloc(NULL, size);
                fread(buf, 1, size, f);
                fclose(f);

                MtBufferReader br;
                mt_buffer_reader_init(&br, buf, size);

                mt_entity_manager_deserialize(em, &br);
            }
        }
    }
    igEnd();
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
