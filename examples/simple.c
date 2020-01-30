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

typedef struct GBuffers
{
    MtDevice *dev;
    MtRenderPass *render_pass;

    MtImage *depth_image;

    MtImage *albedo_image;
    MtImage *metallic_roughness_image;
    MtImage *occlusion_image;
    MtImage *emissive_image;

    MtImage *pos_image;
    MtImage *normal_image;
} GBuffers;

static void gbuffers_resize(GBuffers *pass, uint32_t width, uint32_t height);

static void gbuffers_init(GBuffers *pass, MtDevice *dev, uint32_t width, uint32_t height)
{
    memset(pass, 0, sizeof(*pass));
    pass->dev = dev;
    gbuffers_resize(pass, width, height);
}

static void gbuffers_destroy(GBuffers *pass)
{
    mt_render.destroy_render_pass(pass->dev, pass->render_pass);

    mt_render.destroy_image(pass->dev, pass->depth_image);
    mt_render.destroy_image(pass->dev, pass->albedo_image);
    mt_render.destroy_image(pass->dev, pass->metallic_roughness_image);
    mt_render.destroy_image(pass->dev, pass->emissive_image);
    mt_render.destroy_image(pass->dev, pass->occlusion_image);
    mt_render.destroy_image(pass->dev, pass->pos_image);
    mt_render.destroy_image(pass->dev, pass->normal_image);
}

static void gbuffers_resize(GBuffers *pass, uint32_t width, uint32_t height)
{
    if (pass->render_pass)
    {
        gbuffers_destroy(pass);
    }

    pass->depth_image = mt_render.create_image(
        pass->dev,
        &(MtImageCreateInfo){
            .width  = width,
            .height = height,
            .format = MT_FORMAT_D24_UNORM_S8_UINT,
            .usage  = MT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspect = MT_IMAGE_ASPECT_DEPTH_BIT | MT_IMAGE_ASPECT_STENCIL_BIT,
        });
    MtImageCreateInfo color_create_info = {
        .width  = width,
        .height = height,
        .format = MT_FORMAT_RGBA8_UNORM,
        .usage  = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .aspect = MT_IMAGE_ASPECT_COLOR_BIT,
    };

    pass->albedo_image             = mt_render.create_image(pass->dev, &color_create_info);
    pass->metallic_roughness_image = mt_render.create_image(pass->dev, &color_create_info);
    pass->occlusion_image          = mt_render.create_image(pass->dev, &color_create_info);
    pass->emissive_image           = mt_render.create_image(pass->dev, &color_create_info);

    pass->pos_image = mt_render.create_image(
        pass->dev,
        &(MtImageCreateInfo){
            .width  = width,
            .height = height,
            .format = MT_FORMAT_RGBA16_SFLOAT,
            .usage  = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .aspect = MT_IMAGE_ASPECT_COLOR_BIT,
        });

    pass->normal_image = mt_render.create_image(
        pass->dev,
        &(MtImageCreateInfo){
            .width  = width,
            .height = height,
            .format = MT_FORMAT_RGBA16_SFLOAT,
            .usage  = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .aspect = MT_IMAGE_ASPECT_COLOR_BIT,
        });

    pass->render_pass = mt_render.create_render_pass(
        pass->dev,
        &(MtRenderPassCreateInfo){
            .color_attachment_count = 6,
            .color_attachments      = (MtImage *[]){pass->albedo_image,
                                               pass->metallic_roughness_image,
                                               pass->occlusion_image,
                                               pass->emissive_image,
                                               pass->pos_image,
                                               pass->normal_image},
            .depth_attachment       = pass->depth_image,
        });
}

typedef struct Game
{
    MtEngine engine;

    MtImageAsset *image;
    MtPipelineAsset *pbr_pipeline;
    MtPipelineAsset *pbr_deferred_pipeline;
    MtPipelineAsset *fullscreen_pipeline;
    MtPipelineAsset *gbuffer_pipeline;
    MtPipelineAsset *bloom_blur_pipeline;

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
        &g->engine.asset_manager, "../assets/shaders/pbr.glsl", (MtAsset **)&g->pbr_pipeline);
    mt_asset_manager_queue_load(
        &g->engine.asset_manager,
        "../assets/shaders/pbr_deferred.glsl",
        (MtAsset **)&g->pbr_deferred_pipeline);
    mt_asset_manager_queue_load(
        &g->engine.asset_manager,
        "../assets/shaders/gbuffers.glsl",
        (MtAsset **)&g->gbuffer_pipeline);
    mt_asset_manager_queue_load(
        &g->engine.asset_manager,
        "../assets/shaders/fullscreen.glsl",
        (MtAsset **)&g->fullscreen_pipeline);
    mt_asset_manager_queue_load(
        &g->engine.asset_manager,
        "../assets/shaders/bloom_blur.glsl",
        (MtAsset **)&g->bloom_blur_pipeline);

    mt_asset_manager_queue_load(&g->engine.asset_manager, "../assets/helmet_ktx.glb", NULL);
    mt_asset_manager_queue_load(&g->engine.asset_manager, "../assets/boombox_ktx.glb", NULL);
    mt_asset_manager_queue_load(&g->engine.asset_manager, "../assets/sponza_ktx.glb", NULL);

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

static void light_system(MtEntityArchetype *archetype, MtEnvironment *env, float delta)
{
    static float acc = 0.0f;
    acc += delta;

    float x = sin(acc * 2.0f) * 2.0f;
    float z = cos(acc * 2.0f) * 2.0f;

    env->uniform.point_light_count = 0;
    for (MtEntityBlock *block = archetype->blocks;
         block != (archetype->blocks + mt_array_size(archetype->blocks));
         ++block)
    {
        for (uint32_t i = 0; i < block->entity_count; ++i)
        {
            uint32_t l = env->uniform.point_light_count;

            MtPointLightArchetype *b             = block->data;
            env->uniform.point_lights[l].pos.xyz = b->pos[i];
            env->uniform.point_lights[l].pos.x += x;
            env->uniform.point_lights[l].pos.z += z;
            env->uniform.point_lights[l].pos.w = 1.0f;

            env->uniform.point_lights[l].color = b->color[i];

            float constant  = 1.0;
            float linear    = 0.7;
            float quadratic = 1.8;
            float light_max = fmaxf(fmaxf(b->color[i].r, b->color[i].g), b->color[i].b);
            float radius =
                (-linear +
                 sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * light_max))) /
                (2 * quadratic);

            env->uniform.point_lights[l].radius = radius / 2.0f;

            env->uniform.point_light_count++;
        }
    }
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

            mt_gltf_asset_draw(b->model[i], cb, &transform, 1, 2);
        }
    }
}

static void draw_ui(Game *g)
{
    MtSwapchain *swapchain = g->engine.swapchain;
    MtUIRenderer *ui       = g->engine.ui;

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

    if (mt_ui_button(ui, "Hello"))
    {
        mt_log("Hello");
    }

    if (mt_ui_button(ui, "Hello 2"))
    {
        mt_log("Hello 2");
    }
}

int main(int argc, char *argv[])
{
    Game game = {0};
    game_init(&game);

    MtWindow *win          = game.engine.window;
    MtSwapchain *swapchain = game.engine.swapchain;
    MtDevice *dev          = game.engine.device;
    MtEntityManager *em    = &game.engine.entity_manager;
    MtAssetManager *am     = &game.engine.asset_manager;
    MtUIRenderer *ui       = game.engine.ui;

    uint32_t model_archetype =
        mt_entity_manager_register_archetype(em, mt_model_archetype_init, sizeof(MtModelArchetype));

    uint32_t light_archetype = mt_entity_manager_register_archetype(
        em, mt_point_light_archetype_init, sizeof(MtPointLightArchetype));

    {
        MtModelArchetype *block;
        uint32_t e;

        block           = mt_entity_manager_add_entity(em, model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/helmet_ktx.glb");
        block->pos[e]   = V3(-1.5, 1, 0);

        block           = mt_entity_manager_add_entity(em, model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/boombox_ktx.glb");
        block->scale[e] = V3(100, 100, 100);
        block->pos[e]   = V3(1.5, 1, 0);

        block           = mt_entity_manager_add_entity(em, model_archetype, &e);
        block->model[e] = (MtGltfAsset *)mt_asset_manager_get(am, "../assets/sponza_ktx.glb");
        block->scale[e] = V3(3, 3, 3);
    }

    MtXorShift xs;
    mt_xor_shift_init(&xs, (uint64_t)time(NULL));

    for (uint32_t i = 0; i < MT_MAX_POINT_LIGHTS / 2; ++i)
    {
        MtPointLightArchetype *block;
        uint32_t e;

#define LIGHT_POS_X mt_xor_shift_float(&xs, -15.0f, 15.0f)
#define LIGHT_POS_Z mt_xor_shift_float(&xs, -10.0f, 10.0f)
#define LIGHT_COL mt_xor_shift_float(&xs, 0.0f, 1.0f)

        block           = mt_entity_manager_add_entity(em, light_archetype, &e);
        block->pos[e]   = V3(LIGHT_POS_X, 1, LIGHT_POS_Z);
        block->color[e] = V3(LIGHT_COL, LIGHT_COL, LIGHT_COL);
        block->color[e] = v3_muls(v3_normalize(block->color[e]), 10.0f);
    }

    uint32_t width, height;
    mt_window.get_size(win, &width, &height);

    uint32_t gbuffer_index = 0;
    GBuffers gbuffers[1];
    for (uint32_t i = 0; i < MT_LENGTH(gbuffers); ++i)
    {
        gbuffers_init(&gbuffers[i], dev, width, height);
    }

    while (!mt_window.should_close(win))
    {
        gbuffer_index = (gbuffer_index + 1) % MT_LENGTH(gbuffers);

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
                    for (uint32_t i = 0; i < MT_LENGTH(gbuffers); ++i)
                    {
                        gbuffers_resize(&gbuffers[i], event.size.width, event.size.height);
                    }
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
        float aspect     = (float)width / (float)height;
        float delta_time = mt_render.swapchain_get_delta_time(swapchain);
        mt_perspective_camera_update(&game.cam, win, aspect, delta_time);

        light_system(&em->archetypes[light_archetype], &game.env, delta_time);

        MtCmdBuffer *cb = mt_render.swapchain_begin_frame(swapchain);
        mt_render.begin_cmd_buffer(cb);

        {
            mt_render.cmd_begin_render_pass(cb, gbuffers[gbuffer_index].render_pass, NULL, NULL);

            /* // Draw skybox */
            /* mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0); */
            /* mt_environment_draw_skybox(&game.env, cb); */

            // Draw model
            mt_render.cmd_bind_pipeline(cb, game.gbuffer_pipeline->pipeline);
            mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);
            model_system(cb, &em->archetypes[model_archetype]);

            mt_render.cmd_end_render_pass(cb);
        }

        {
            mt_render.cmd_begin_render_pass(
                cb, mt_render.swapchain_get_render_pass(swapchain), NULL, NULL);

            mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);
            mt_environment_draw_skybox(&game.env, cb);

            GBuffers *gb = &gbuffers[gbuffer_index];
            mt_render.cmd_bind_pipeline(cb, game.pbr_deferred_pipeline->pipeline);

            mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);

            mt_environment_bind(&game.env, cb, 1);

            mt_render.cmd_bind_image(cb, gb->albedo_image, game.engine.default_sampler, 2, 0);
            mt_render.cmd_bind_image(
                cb, gb->metallic_roughness_image, game.engine.default_sampler, 2, 1);
            mt_render.cmd_bind_image(cb, gb->occlusion_image, game.engine.default_sampler, 2, 2);
            mt_render.cmd_bind_image(cb, gb->emissive_image, game.engine.default_sampler, 2, 3);
            mt_render.cmd_bind_image(cb, gb->normal_image, game.engine.default_sampler, 2, 4);
            mt_render.cmd_bind_image(cb, gb->pos_image, game.engine.default_sampler, 2, 5);

            mt_render.cmd_draw(cb, 3, 1, 0, 0);

            /* mt_render.cmd_bind_pipeline(cb, game.fullscreen_pipeline->pipeline); */
            /* mt_render.cmd_bind_image(cb, gb->metallic_roughness_image,
             * game.engine.default_sampler, 0, 0); */
            /* mt_render.cmd_draw(cb, 3, 1, 0, 0); */

            // Begin UI
            MtViewport viewport;
            mt_render.cmd_get_viewport(cb, &viewport);
            mt_ui_begin(ui, &viewport);

            // Submit UI commands
            draw_ui(&game);

            /* mt_ui_image(ui, gb->albedo_image, 64, 64); */
            /* mt_ui_image(ui, gb->metallic_roughness_image, 64, 64); */
            /* mt_ui_image(ui, gb->occlusion_image, 64, 64); */
            /* mt_ui_image(ui, gb->emissive_image, 64, 64); */
            /* mt_ui_image(ui, gb->normal_image, 64, 64); */
            /* mt_ui_image(ui, gb->pos_image, 64, 64); */

            // Draw UI
            mt_ui_draw(ui, cb);

            mt_render.cmd_end_render_pass(cb);
        }

        mt_render.end_cmd_buffer(cb);
        mt_render.swapchain_end_frame(swapchain);
    }

    for (uint32_t i = 0; i < MT_LENGTH(gbuffers); ++i)
    {
        gbuffers_destroy(&gbuffers[i]);
    }
    game_destroy(&game);

    return 0;
}
