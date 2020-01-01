#include "../../../include/motor/assets/pipeline_asset.h"

#include "../../../include/motor/util.h"
#include "../../../include/motor/allocator.h"
#include "../../../include/motor/asset_manager.h"
#include "../../../include/motor/engine.h"
#include "../../../include/motor/renderer.h"
#include "../../../include/motor/array.h"
#include "../../../include/motor/config.h"
#include "../../../include/motor/math_types.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <shaderc/shaderc.h>

static void asset_destroy(MtAsset *asset_) {
    MtPipelineAsset *asset = (MtPipelineAsset *)asset_;
    if (!asset) return;

    mt_render.destroy_pipeline(
        asset->asset_manager->engine->device, asset->pipeline);
}

bool asset_init(
    MtAssetManager *asset_manager, MtAsset *asset_, const char *path) {
    MtPipelineAsset *asset = (MtPipelineAsset *)asset_;
    asset->asset_manager   = asset_manager;

    // Read file
    FILE *f = fopen(path, "r");
    if (!f) {
        goto failed;
    }

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *input = mt_alloc(asset_manager->alloc, input_size);
    fread(input, input_size, 1, f);

    fclose(f);

    shaderc_compile_options_t options = NULL;
    char *vertex_text = NULL, *fragment_text = NULL;

    shaderc_compilation_result_t vertex_result = NULL, fragment_result = NULL;

    // Parse file
    MtConfig *config = mt_config_parse(asset_manager->alloc, input, input_size);
    if (!config) {
        goto failed;
    }

    options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_optimization_level(
        options, shaderc_optimization_level_performance);
    shaderc_compile_options_set_forced_version_profile(
        options, 450, shaderc_profile_none);
    shaderc_compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

    MtConfigObject *obj = mt_config_get_root(config);
    MtConfigEntry *vertex_entry =
        mt_hash_get_ptr(&obj->map, mt_hash_str("vertex"));
    MtConfigEntry *fragment_entry =
        mt_hash_get_ptr(&obj->map, mt_hash_str("fragment"));
    MtConfigEntry *common_entry =
        mt_hash_get_ptr(&obj->map, mt_hash_str("common"));

    if (!vertex_entry || !fragment_entry) {
        goto failed;
    }

    if (vertex_entry->value.type != MT_CONFIG_VALUE_STRING ||
        fragment_entry->value.type != MT_CONFIG_VALUE_STRING) {
        goto failed;
    }

    vertex_text = mt_strdup(asset_manager->alloc, vertex_entry->value.string);
    fragment_text =
        mt_strdup(asset_manager->alloc, fragment_entry->value.string);

    if (common_entry && common_entry->value.type == MT_CONFIG_VALUE_STRING) {
        vertex_text = mt_strcat(
            asset_manager->alloc, common_entry->value.string, vertex_text);
        fragment_text = mt_strcat(
            asset_manager->alloc, common_entry->value.string, fragment_text);
    }

    size_t vertex_text_size   = strlen(vertex_text);
    size_t fragment_text_size = strlen(fragment_text);

    // Compile vertex shader
    vertex_result = shaderc_compile_into_spv(
        asset_manager->engine->compiler,
        vertex_text,
        vertex_text_size,
        shaderc_vertex_shader,
        path,
        "main",
        options);

    if (shaderc_result_get_compilation_status(vertex_result) ==
        shaderc_compilation_status_compilation_error) {
        printf("%s\n", shaderc_result_get_error_message(vertex_result));
        goto failed;
    }

    // Compile fragment shader
    fragment_result = shaderc_compile_into_spv(
        asset_manager->engine->compiler,
        fragment_text,
        fragment_text_size,
        shaderc_fragment_shader,
        path,
        "main",
        options);

    if (shaderc_result_get_compilation_status(fragment_result) ==
        shaderc_compilation_status_compilation_error) {
        printf("%s\n", shaderc_result_get_error_message(fragment_result));
        goto failed;
    }

    asset->pipeline = mt_render.create_graphics_pipeline(
        asset_manager->engine->device,
        (uint8_t *)shaderc_result_get_bytes(vertex_result),
        shaderc_result_get_length(vertex_result),
        (uint8_t *)shaderc_result_get_bytes(fragment_result),
        shaderc_result_get_length(fragment_result),
        &(MtGraphicsPipelineCreateInfo){
            .cull_mode  = MT_CULL_MODE_NONE,
            .front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE,
        });

    shaderc_compile_options_release(options);
    mt_config_destroy(config);
    mt_free(asset_manager->alloc, input);

    shaderc_result_release(vertex_result);
    shaderc_result_release(fragment_result);

    return true;

failed:
    if (options) shaderc_compile_options_release(options);
    if (config) mt_config_destroy(config);
    if (input) mt_free(asset_manager->alloc, input);

    if (vertex_result) shaderc_result_release(vertex_result);
    if (fragment_result) shaderc_result_release(fragment_result);
    return false;
}

static const char *g_extensions[] = {
    ".glsl",
};

static MtAssetVT g_asset_vt = {
    .name            = "Pipeline",
    .extensions      = g_extensions,
    .extension_count = MT_LENGTH(g_extensions),
    .size            = sizeof(MtPipelineAsset),
    .init            = asset_init,
    .destroy         = asset_destroy,
};
MtAssetVT *mt_pipeline_asset_vt = &g_asset_vt;
