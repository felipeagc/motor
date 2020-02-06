#include <stdio.h>
#include <string.h>
#include <shaderc/shaderc.h>
#include <motor/base/string_builder.h>
#include <motor/base/log.h>
#include <motor/engine/config.h>

static shaderc_include_result *include_resolver(
    void *user_data,
    const char *requested_source,
    int type,
    const char *requesting_source,
    size_t include_depth)
{
    MtEngine *engine   = user_data;
    MtAllocator *alloc = engine->alloc;

    assert(type == shaderc_include_type_relative);

    uint64_t last_slash = 0;
    for (uint64_t i = 0; i < strlen(requesting_source); i++)
    {
        if (requesting_source[i] == '/')
            last_slash = i + 1;
    }

    char *path = NULL;
    if (last_slash > 0)
    {
        path = mt_alloc(alloc, last_slash + 1);
        memcpy(path, requesting_source, last_slash);
        path[last_slash] = '\0';
    }
    path = mt_strcat(alloc, path, requested_source);

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        mt_log_error("Could not open shader include: %s", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    uint64_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = mt_alloc(alloc, size);
    fread(content, size, 1, f);

    fclose(f);

    shaderc_include_result *result = mt_alloc(alloc, sizeof(*result));
    *result                        = (shaderc_include_result){
        .source_name        = path,
        .source_name_length = strlen(path),
        .content            = content,
        .content_length     = size,
        .user_data          = user_data,
    };

    return result;
}

// An includer callback type for destroying an include result.
static void include_result_releaser(void *user_data, shaderc_include_result *include_result)
{
    MtEngine *engine   = user_data;
    MtAllocator *alloc = engine->alloc;

    mt_free(alloc, (void *)include_result->content);
    mt_free(alloc, (void *)include_result->source_name);
    mt_free(alloc, include_result);
}

static MtPipeline *create_compute_pipeline(MtEngine *engine, const char *path, MtConfig *config)
{
    MtConfigObject *obj         = mt_config_get_root(config);
    MtConfigEntry *common_entry = mt_hash_get_ptr(&obj->map, mt_hash_str("common"));
    MtConfigEntry *comp_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("compute"));

    shaderc_compile_options_t options        = NULL;
    char *comp_text                          = NULL;
    shaderc_compilation_result_t comp_result = NULL;

    options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    shaderc_compile_options_set_forced_version_profile(options, 450, shaderc_profile_none);

    shaderc_compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

    shaderc_compile_options_set_include_callbacks(
        options, include_resolver, include_result_releaser, engine);

    if (comp_entry->value.type != MT_CONFIG_VALUE_STRING)
    {
        mt_log_error("Pipeline asset requires \"compute\"  to be a string");
        goto failed;
    }

    MtStringBuilder sb;
    mt_str_builder_init(&sb, engine->alloc);

    if (common_entry)
    {
        mt_str_builder_append_str(&sb, common_entry->value.string);
    }
    mt_str_builder_append_strn(&sb, comp_entry->value.string, comp_entry->value.length);
    comp_text = mt_str_builder_build(&sb, engine->alloc);

    mt_str_builder_destroy(&sb);

    size_t comp_text_size = strlen(comp_text);

    // Compile vertex shader
    comp_result = shaderc_compile_into_spv(
        engine->compiler, comp_text, comp_text_size, shaderc_compute_shader, path, "main", options);

    if (shaderc_result_get_compilation_status(comp_result) ==
        shaderc_compilation_status_compilation_error)
    {
        mt_log_error("%s", shaderc_result_get_error_message(comp_result));
        goto failed;
    }

    MtPipeline *pipeline = mt_render.create_compute_pipeline(
        engine->device,
        (uint8_t *)shaderc_result_get_bytes(comp_result),
        shaderc_result_get_length(comp_result));

    shaderc_compile_options_release(options);

    shaderc_result_release(comp_result);

    return pipeline;

failed:
    if (options)
        shaderc_compile_options_release(options);

    if (comp_result)
        shaderc_result_release(comp_result);

    return NULL;
}

static MtPipeline *create_graphics_pipeline(MtEngine *engine, const char *path, MtConfig *config)
{
    MtConfigObject *obj = mt_config_get_root(config);

    MtConfigEntry *common_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("common"));
    MtConfigEntry *vertex_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("vertex"));
    MtConfigEntry *fragment_entry = mt_hash_get_ptr(&obj->map, mt_hash_str("fragment"));

    MtConfigEntry *blending_entry    = mt_hash_get_ptr(&obj->map, mt_hash_str("blending"));
    MtConfigEntry *depth_test_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("depth_test"));
    MtConfigEntry *depth_write_entry = mt_hash_get_ptr(&obj->map, mt_hash_str("depth_write"));
    MtConfigEntry *depth_bias_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("depth_bias"));
    MtConfigEntry *cull_mode_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("cull_mode"));
    MtConfigEntry *front_face_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("front_face"));
    MtConfigEntry *line_width_entry  = mt_hash_get_ptr(&obj->map, mt_hash_str("line_width"));

    shaderc_compile_options_t options = NULL;
    char *vertex_text = NULL, *fragment_text = NULL;
    size_t vertex_text_size = 0, fragment_text_size = 0;
    shaderc_compilation_result_t vertex_result = NULL, fragment_result = NULL;

    options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    shaderc_compile_options_set_forced_version_profile(options, 450, shaderc_profile_none);
    shaderc_compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
    shaderc_compile_options_set_include_callbacks(
        options, include_resolver, include_result_releaser, engine);

    if (vertex_entry && vertex_entry->value.type != MT_CONFIG_VALUE_STRING)
    {
        mt_log_error("Pipeline asset requires \"vertex\" field to be a string");
        goto failed;
    }

    if (fragment_entry && fragment_entry->value.type != MT_CONFIG_VALUE_STRING)
    {
        mt_log_error("Pipeline asset requires \"fragment\" field to be a string");
        goto failed;
    }

    uint8_t *vertex_code = NULL, *fragment_code = NULL;
    size_t vertex_code_size = 0, fragment_code_size = 0;

    MtStringBuilder sb;
    mt_str_builder_init(&sb, engine->alloc);

    if (vertex_entry)
    {
        mt_str_builder_reset(&sb);
        if (common_entry)
        {
            mt_str_builder_append_str(&sb, common_entry->value.string);
        }
        mt_str_builder_append_strn(&sb, vertex_entry->value.string, vertex_entry->value.length);
        vertex_text      = mt_str_builder_build(&sb, engine->alloc);
        vertex_text_size = strlen(vertex_text);

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
            mt_log_error("%s", shaderc_result_get_error_message(vertex_result));
            goto failed;
        }

        vertex_code      = (uint8_t *)shaderc_result_get_bytes(vertex_result);
        vertex_code_size = shaderc_result_get_length(vertex_result);
    }

    if (fragment_entry)
    {
        mt_str_builder_reset(&sb);
        if (common_entry)
        {
            mt_str_builder_append_str(&sb, common_entry->value.string);
        }
        mt_str_builder_append_strn(&sb, fragment_entry->value.string, fragment_entry->value.length);
        fragment_text      = mt_str_builder_build(&sb, engine->alloc);
        fragment_text_size = strlen(fragment_text);

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
            mt_log_error("%s", shaderc_result_get_error_message(fragment_result));
            goto failed;
        }

        fragment_code      = (uint8_t *)shaderc_result_get_bytes(fragment_result);
        fragment_code_size = shaderc_result_get_length(fragment_result);
    }

    mt_str_builder_destroy(&sb);

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
            mt_log_error("Invalid pipeline blending type, wanted bool");
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
            mt_log_error("Invalid pipeline depth test type, wanted bool");
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
            mt_log_error("Invalid pipeline depth write type, wanted bool");
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
            mt_log_error("Invalid pipeline depth bias type, wanted bool");
            goto failed;
        }
    }

    if (front_face_entry)
    {
        MtConfigValue *value = &front_face_entry->value;
        if (value->type == MT_CONFIG_VALUE_STRING)
        {
            if (strncmp(value->string, "clockwise", value->length) == 0)
            {
                front_face = MT_FRONT_FACE_CLOCKWISE;
            }
            else if (strncmp(value->string, "counter_clockwise", value->length) == 0)
            {
                front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE;
            }
            else
            {
                mt_log_error("Invalid pipeline front face: \"%.*s\"", value->length, value->string);
                goto failed;
            }
        }
        else
        {
            mt_log_error("Invalid pipeline front face type, wanted string");
            goto failed;
        }
    }

    if (cull_mode_entry)
    {
        MtConfigValue *value = &cull_mode_entry->value;
        if (value->type == MT_CONFIG_VALUE_STRING)
        {
            if (strncmp(value->string, "none", value->length) == 0)
            {
                cull_mode = MT_CULL_MODE_NONE;
            }
            else if (strncmp(value->string, "front", value->length) == 0)
            {
                cull_mode = MT_CULL_MODE_FRONT;
            }
            else if (strncmp(value->string, "back", value->length) == 0)
            {
                cull_mode = MT_CULL_MODE_BACK;
            }
            else
            {
                mt_log_error("Invalid pipeline cull mode: \"%.*s\"", value->length, value->string);
                goto failed;
            }
        }
        else
        {
            mt_log_error("Invalid pipeline cull mode type, wanted string");
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
            mt_log_error("Invalid pipeline line width type, wanted float");
            goto failed;
        }
    }

    MtPipeline *pipeline = mt_render.create_graphics_pipeline(
        engine->device,
        vertex_code,
        vertex_code_size,
        fragment_code,
        fragment_code_size,
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

    shaderc_result_release(vertex_result);
    shaderc_result_release(fragment_result);

    return pipeline;

failed:
    if (options)
        shaderc_compile_options_release(options);

    if (vertex_result)
        shaderc_result_release(vertex_result);
    if (fragment_result)
        shaderc_result_release(fragment_result);
    return NULL;
}

static MtPipeline *
create_pipeline(MtEngine *engine, const char *path, const char *input, size_t input_size)
{
    // Parse file
    MtConfig *config = mt_config_parse(engine->alloc, input, input_size);
    if (!config)
    {
        mt_log_error("Failed to parse pipeline config: \"%s\"", path);
        return NULL;
    }

    MtConfigObject *obj           = mt_config_get_root(config);
    MtConfigEntry *vertex_entry   = mt_hash_get_ptr(&obj->map, mt_hash_str("vertex"));
    MtConfigEntry *fragment_entry = mt_hash_get_ptr(&obj->map, mt_hash_str("fragment"));
    MtConfigEntry *comp_entry     = mt_hash_get_ptr(&obj->map, mt_hash_str("compute"));

    MtPipeline *pipeline = NULL;

    if (comp_entry && !vertex_entry && !fragment_entry)
    {
        pipeline = create_compute_pipeline(engine, path, config);
    }
    else if (!comp_entry && vertex_entry)
    {
        pipeline = create_graphics_pipeline(engine, path, config);
    }
    else
    {
        mt_log_error("Failed to create pipeline: \"%s\", missing required shader fields", path);
    }

    mt_config_destroy(config);
    return pipeline;
}
