#include <stdio.h>
#include <string.h>
#include <shaderc/shaderc.h>
#include <motor/base/string_builder.h>
#include <motor/engine/config.h>

static MtPipeline *
create_pipeline(MtEngine *engine, const char *path, const char *input, size_t input_size)
{
    shaderc_compile_options_t options = NULL;
    char *vertex_text = NULL, *fragment_text = NULL;

    shaderc_compilation_result_t vertex_result = NULL, fragment_result = NULL;

    // Parse file
    MtConfig *config = mt_config_parse(engine->alloc, input, input_size);
    if (!config)
    {
        goto failed;
    }

    options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    shaderc_compile_options_set_forced_version_profile(options, 450, shaderc_profile_none);
    shaderc_compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

    MtConfigObject *obj           = mt_config_get_root(config);
    MtConfigEntry *vertex_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("vertex"));
    MtConfigEntry *fragment_entry = mt_hash_get_ptr(&obj->map, mt_hash_str("fragment"));
    MtConfigEntry *common_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("common"));

    MtConfigEntry *blending_entry    = mt_hash_get_ptr(&obj->map, mt_hash_str("blending"));
    MtConfigEntry *depth_test_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("depth_test"));
    MtConfigEntry *depth_write_entry = mt_hash_get_ptr(&obj->map, mt_hash_str("depth_write"));
    MtConfigEntry *depth_bias_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("depth_bias"));
    MtConfigEntry *cull_mode_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("cull_mode"));
    MtConfigEntry *front_face_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("front_face"));
    MtConfigEntry *line_width_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("line_width"));

    if (!vertex_entry || !fragment_entry)
    {
        printf("Pipeline asset requires \"vertex\" and \"fragment\" properties\n");
        goto failed;
    }

    if (vertex_entry->value.type != MT_CONFIG_VALUE_STRING ||
        fragment_entry->value.type != MT_CONFIG_VALUE_STRING)
    {
        printf("Pipeline asset requires \"vertex\" and \"fragment\" to be "
               "strings\n");
        goto failed;
    }

    MtStringBuilder sb;
    mt_str_builder_init(&sb, engine->alloc);

    if (common_entry)
    {
        mt_str_builder_append_str(&sb, common_entry->value.string);
    }
    mt_str_builder_append_str(&sb, vertex_entry->value.string);
    vertex_text = mt_str_builder_build(&sb, engine->alloc);

    mt_str_builder_reset(&sb);

    if (common_entry)
    {
        mt_str_builder_append_str(&sb, common_entry->value.string);
    }
    mt_str_builder_append_str(&sb, fragment_entry->value.string);
    fragment_text = mt_str_builder_build(&sb, engine->alloc);

    mt_str_builder_destroy(&sb);

    size_t vertex_text_size   = strlen(vertex_text);
    size_t fragment_text_size = strlen(fragment_text);

    // Compile vertex shader
    vertex_result = shaderc_compile_into_spv(
        engine->compiler,
        vertex_text,
        vertex_text_size,
        shaderc_vertex_shader,
        path,
        "main",
        options);

    if (shaderc_result_get_compilation_status(vertex_result) ==
        shaderc_compilation_status_compilation_error)
    {
        printf("%s\n", shaderc_result_get_error_message(vertex_result));
        goto failed;
    }

    // Compile fragment shader
    fragment_result = shaderc_compile_into_spv(
        engine->compiler,
        fragment_text,
        fragment_text_size,
        shaderc_fragment_shader,
        path,
        "main",
        options);

    if (shaderc_result_get_compilation_status(fragment_result) ==
        shaderc_compilation_status_compilation_error)
    {
        printf("%s\n", shaderc_result_get_error_message(fragment_result));
        goto failed;
    }

    bool blending          = true;
    bool depth_test        = true;
    bool depth_write       = true;
    bool depth_bias        = false;
    MtFrontFace front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE;
    MtCullMode cull_mode   = MT_CULL_MODE_NONE;
    float line_width       = 1.0f;

    if (blending_entry)
    {
        if (blending_entry->value.type == MT_CONFIG_VALUE_BOOL)
        {
            blending = blending_entry->value.boolean;
        }
        else
        {
            printf("Invalid pipeline blending type, wanted bool\n");
            goto failed;
        }
    }

    if (depth_test_entry)
    {
        if (depth_test_entry->value.type == MT_CONFIG_VALUE_BOOL)
        {
            depth_test = depth_test_entry->value.boolean;
        }
        else
        {
            printf("Invalid pipeline depth test type, wanted bool\n");
            goto failed;
        }
    }

    if (depth_write_entry)
    {
        if (depth_write_entry->value.type == MT_CONFIG_VALUE_BOOL)
        {
            depth_write = depth_write_entry->value.boolean;
        }
        else
        {
            printf("Invalid pipeline depth write type, wanted bool\n");
            goto failed;
        }
    }

    if (depth_bias_entry)
    {
        if (depth_bias_entry->value.type == MT_CONFIG_VALUE_BOOL)
        {
            depth_bias = depth_bias_entry->value.boolean;
        }
        else
        {
            printf("Invalid pipeline depth bias type, wanted bool\n");
            goto failed;
        }
    }

    if (front_face_entry)
    {
        if (front_face_entry->value.type == MT_CONFIG_VALUE_STRING)
        {
            if (strcmp(front_face_entry->value.string, "clockwise") == 0)
            {
                front_face = MT_FRONT_FACE_CLOCKWISE;
            }
            else if (strcmp(front_face_entry->value.string, "counter_clockwise") == 0)
            {
                front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE;
            }
            else
            {
                printf("Invalid pipeline front face: \"%s\"\n", front_face_entry->value.string);
                goto failed;
            }
        }
        else
        {
            printf("Invalid pipeline front face type, wanted string\n");
            goto failed;
        }
    }

    if (cull_mode_entry)
    {
        if (cull_mode_entry->value.type == MT_CONFIG_VALUE_STRING)
        {
            if (strcmp(cull_mode_entry->value.string, "none") == 0)
            {
                cull_mode = MT_CULL_MODE_NONE;
            }
            else if (strcmp(cull_mode_entry->value.string, "front") == 0)
            {
                cull_mode = MT_CULL_MODE_FRONT;
            }
            else if (strcmp(cull_mode_entry->value.string, "back") == 0)
            {
                cull_mode = MT_CULL_MODE_BACK;
            }
            else
            {
                printf("Invalid pipeline cull mode: \"%s\"\n", cull_mode_entry->value.string);
                goto failed;
            }
        }
        else
        {
            printf("Invalid pipeline cull mode type, wanted string\n");
            goto failed;
        }
    }

    if (line_width_entry)
    {
        if (line_width_entry->value.type == MT_CONFIG_VALUE_FLOAT)
        {
            line_width = (float)line_width_entry->value.f64;
        }
        else
        {
            printf("Invalid pipeline line width type, wanted float\n");
            goto failed;
        }
    }

    MtPipeline *pipeline = mt_render.create_graphics_pipeline(
        engine->device,
        (uint8_t *)shaderc_result_get_bytes(vertex_result),
        shaderc_result_get_length(vertex_result),
        (uint8_t *)shaderc_result_get_bytes(fragment_result),
        shaderc_result_get_length(fragment_result),
        &(MtGraphicsPipelineCreateInfo){
            .blending    = blending,
            .depth_test  = depth_test,
            .depth_write = depth_write,
            .depth_bias  = depth_bias,
            .cull_mode   = cull_mode,
            .front_face  = front_face,
            .line_width  = line_width,
        });

    shaderc_compile_options_release(options);
    mt_config_destroy(config);

    shaderc_result_release(vertex_result);
    shaderc_result_release(fragment_result);

    return pipeline;

failed:
    if (options)
        shaderc_compile_options_release(options);
    if (config)
        mt_config_destroy(config);

    if (vertex_result)
        shaderc_result_release(vertex_result);
    if (fragment_result)
        shaderc_result_release(fragment_result);
    return NULL;
}
