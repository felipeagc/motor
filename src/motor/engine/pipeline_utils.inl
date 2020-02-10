#include <stdio.h>
#include <string.h>
#include <shaderc/shaderc.h>
#include <motor/base/string_builder.h>
#include <motor/base/log.h>
#include <motor/base/array.h>
#include <motor/engine/config.h>

typedef struct PipelinePragma
{
    const char *key;
    uint32_t key_len;
    const char *value;
    uint32_t value_len;
} PipelinePragma;

static PipelinePragma *parse_pragmas(MtAllocator *alloc, const char *input, size_t input_size)
{
    PipelinePragma *pragmas = NULL;

    const char *c = input;
    do
    {
        if (*c == '#')
        {
            const char *pragma_prefix = "#pragma motor ";
            if (strncmp(c, pragma_prefix, strlen(pragma_prefix)) == 0)
            {
                c += strlen(pragma_prefix);
                PipelinePragma pragma;
                pragma.key_len = 0;
                pragma.key = c;
                while (*c && *c != ' ' && (c - input) < input_size)
                {
                    pragma.key_len++;
                    c++;
                }

                if (!(*c) || (c - input) >= input_size)
                {
                    return pragmas;
                }

                assert(*c == ' ');
                c++;

                pragma.value_len = 0;
                pragma.value = c;
                while (*c && *c != '\n' && (c - input) < input_size)
                {
                    pragma.value_len++;
                    c++;
                }
                mt_array_push(alloc, pragmas, pragma);
            }
        }
    } while (*c++ && (c - input) < input_size);

    return pragmas;
}

// Include resolver {{{
static shaderc_include_result *include_resolver(
    void *user_data,
    const char *requested_source,
    int type,
    const char *requesting_source,
    size_t include_depth)
{
    MtEngine *engine = user_data;
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
    *result = (shaderc_include_result){
        .source_name = path,
        .source_name_length = strlen(path),
        .content = content,
        .content_length = size,
        .user_data = user_data,
    };

    return result;
}

// An includer callback type for destroying an include result.
static void include_result_releaser(void *user_data, shaderc_include_result *include_result)
{
    MtEngine *engine = user_data;
    MtAllocator *alloc = engine->alloc;

    mt_free(alloc, (void *)include_result->content);
    mt_free(alloc, (void *)include_result->source_name);
    mt_free(alloc, include_result);
}
// }}}

// HLSL graphics pipeline {{{
static MtPipeline *create_graphics_pipeline_hlsl(
    MtEngine *engine,
    const char *path,
    const char *input,
    size_t input_size,
    PipelinePragma *pragmas)
{
    MtGraphicsPipelineCreateInfo pipeline_create_info = {
        .blending = true,
        .depth_test = true,
        .depth_write = true,
        .depth_bias = false,
        .front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE,
        .cull_mode = MT_CULL_MODE_NONE,
        .line_width = 1.0f,
    };

    char *vertex_entry_point = NULL;
    char *fragment_entry_point = NULL;

    for (PipelinePragma *pragma = pragmas; pragma != pragmas + mt_array_size(pragmas); ++pragma)
    {
        if (strncmp(pragma->key, "vertex_entry", pragma->key_len) == 0)
        {
            vertex_entry_point = mt_strndup(engine->alloc, pragma->value, pragma->value_len + 1);
        }

        if (strncmp(pragma->key, "pixel_entry", pragma->key_len) == 0)
        {
            fragment_entry_point = mt_strndup(engine->alloc, pragma->value, pragma->value_len + 1);
        }

        if (strncmp(pragma->key, "blending", pragma->key_len) == 0)
        {
            pipeline_create_info.blending = strncmp(pragma->value, "true", pragma->value_len) == 0;
        }

        if (strncmp(pragma->key, "depth_test", pragma->key_len) == 0)
        {
            pipeline_create_info.depth_test =
                strncmp(pragma->value, "true", pragma->value_len) == 0;
        }

        if (strncmp(pragma->key, "depth_write", pragma->key_len) == 0)
        {
            pipeline_create_info.depth_write =
                strncmp(pragma->value, "true", pragma->value_len) == 0;
        }

        if (strncmp(pragma->key, "depth_bias", pragma->key_len) == 0)
        {
            pipeline_create_info.depth_bias =
                strncmp(pragma->value, "true", pragma->value_len) == 0;
        }

        if (strncmp(pragma->key, "front_face", pragma->key_len) == 0)
        {
            if (strncmp(pragma->value, "clockwise", pragma->value_len) == 0)
            {
                pipeline_create_info.front_face = MT_FRONT_FACE_CLOCKWISE;
            }
            else if (strncmp(pragma->value, "counter_clockwise", pragma->value_len) == 0)
            {
                pipeline_create_info.front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE;
            }
        }

        if (strncmp(pragma->key, "cull_mode", pragma->key_len) == 0)
        {
            if (strncmp(pragma->value, "none", pragma->value_len) == 0)
            {
                pipeline_create_info.cull_mode = MT_CULL_MODE_NONE;
            }
            else if (strncmp(pragma->value, "back", pragma->value_len) == 0)
            {
                pipeline_create_info.cull_mode = MT_CULL_MODE_BACK;
            }
            else if (strncmp(pragma->value, "front", pragma->value_len) == 0)
            {
                pipeline_create_info.cull_mode = MT_CULL_MODE_FRONT;
            }
            else if (strncmp(pragma->value, "front_and_back", pragma->value_len) == 0)
            {
                pipeline_create_info.cull_mode = MT_CULL_MODE_FRONT_AND_BACK;
            }
        }

        if (strncmp(pragma->key, "line_width", pragma->key_len) == 0)
        {
            char str[48];
            assert(pragma->value_len < sizeof(str));
            memcpy(str, pragma->value, pragma->value_len);
            str[pragma->value_len] = '\0';
            pipeline_create_info.line_width = (float)strtod(str, NULL);
        }
    }

    if (!vertex_entry_point)
    {
        mt_log_error("Missing vertex shader entry point");
        return NULL;
    }

    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    shaderc_compile_options_set_forced_version_profile(options, 450, shaderc_profile_none);
    shaderc_compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
    shaderc_compile_options_set_include_callbacks(
        options, include_resolver, include_result_releaser, engine);
    shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);

    shaderc_compilation_result_t vertex_result = NULL, fragment_result = NULL;

    uint8_t *vertex_code = NULL, *fragment_code = NULL;
    size_t vertex_code_size = 0, fragment_code_size = 0;

    vertex_result = shaderc_compile_into_spv(
        engine->compiler,
        input,
        input_size,
        shaderc_vertex_shader,
        path,
        vertex_entry_point,
        options);

    if (shaderc_result_get_compilation_status(vertex_result) ==
        shaderc_compilation_status_compilation_error)
    {
        mt_log_error("%s", shaderc_result_get_error_message(vertex_result));
        goto failed;
    }

    vertex_code = (uint8_t *)shaderc_result_get_bytes(vertex_result);
    vertex_code_size = shaderc_result_get_length(vertex_result);

    // Compile fragment shader
    if (fragment_entry_point)
    {
        fragment_result = shaderc_compile_into_spv(
            engine->compiler,
            input,
            input_size,
            shaderc_fragment_shader,
            path,
            fragment_entry_point,
            options);

        if (shaderc_result_get_compilation_status(fragment_result) ==
            shaderc_compilation_status_compilation_error)
        {
            mt_log_error("%s", shaderc_result_get_error_message(fragment_result));
            goto failed;
        }

        fragment_code = (uint8_t *)shaderc_result_get_bytes(fragment_result);
        fragment_code_size = shaderc_result_get_length(fragment_result);
    }

    mt_render.device_wait_idle(engine->device);
    MtPipeline *pipeline = mt_render.create_graphics_pipeline(
        engine->device,
        vertex_code,
        vertex_code_size,
        fragment_code,
        fragment_code_size,
        &pipeline_create_info);

    shaderc_compile_options_release(options);

    shaderc_result_release(vertex_result);
    shaderc_result_release(fragment_result);

    mt_free(engine->alloc, vertex_entry_point);
    mt_free(engine->alloc, fragment_entry_point);

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
// }}}

// HLSL compute pipeline {{{
static MtPipeline *create_compute_pipeline_hlsl(
    MtEngine *engine,
    const char *path,
    const char *input,
    size_t input_size,
    PipelinePragma *pragmas)
{
    char *compute_entry_point = NULL;

    for (PipelinePragma *pragma = pragmas; pragma != pragmas + mt_array_size(pragmas); ++pragma)
    {
        if (strncmp(pragma->key, "compute_entry", pragma->key_len) == 0)
        {
            compute_entry_point = mt_strndup(engine->alloc, pragma->value, pragma->value_len + 1);
        }
    }

    if (!compute_entry_point)
    {
        mt_log_error("Missing shader compute entry point");
        return NULL;
    }

    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    shaderc_compile_options_set_forced_version_profile(options, 450, shaderc_profile_none);
    shaderc_compile_options_set_target_env(
        options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
    shaderc_compile_options_set_include_callbacks(
        options, include_resolver, include_result_releaser, engine);
    shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);

    shaderc_compilation_result_t compute_result = NULL;

    uint8_t *compute_code = NULL;
    size_t compute_code_size = 0;

    compute_result = shaderc_compile_into_spv(
        engine->compiler,
        input,
        input_size,
        shaderc_compute_shader,
        path,
        compute_entry_point,
        options);

    if (shaderc_result_get_compilation_status(compute_result) ==
        shaderc_compilation_status_compilation_error)
    {
        mt_log_error("%s", shaderc_result_get_error_message(compute_result));
        goto failed;
    }

    compute_code = (uint8_t *)shaderc_result_get_bytes(compute_result);
    compute_code_size = shaderc_result_get_length(compute_result);

    mt_render.device_wait_idle(engine->device);
    MtPipeline *pipeline =
        mt_render.create_compute_pipeline(engine->device, compute_code, compute_code_size);

    shaderc_compile_options_release(options);

    shaderc_result_release(compute_result);

    mt_free(engine->alloc, compute_entry_point);

    return pipeline;

failed:
    if (options)
        shaderc_compile_options_release(options);

    if (compute_result)
        shaderc_result_release(compute_result);
    return NULL;
}
// }}}

static MtPipeline *
create_pipeline(MtEngine *engine, const char *path, const char *input, size_t input_size)
{
    assert(strlen(path) > 0);
    const char *ext = mt_path_ext(path);

    if (strcmp(ext, ".hlsl") == 0)
    {
        PipelinePragma *pragmas = parse_pragmas(engine->alloc, input, input_size);

        MtPipeline *pipeline = NULL;

        enum
        {
            NONE,
            GRAPHICS,
            COMPUTE,
        } type = GRAPHICS;

        for (PipelinePragma *pragma = pragmas; pragma != pragmas + mt_array_size(pragmas); ++pragma)
        {
            if (strncmp(pragma->key, "vertex_entry", pragma->key_len) == 0)
            {
                type = GRAPHICS;
            }
            if (strncmp(pragma->key, "pixel_entry", pragma->key_len) == 0)
            {
                type = GRAPHICS;
            }
            if (strncmp(pragma->key, "compute_entry", pragma->key_len) == 0)
            {
                type = COMPUTE;
            }
        }

        switch (type)
        {
            case GRAPHICS:
            {
                pipeline = create_graphics_pipeline_hlsl(engine, path, input, input_size, pragmas);
                break;
            }
            case COMPUTE:
            {
                pipeline = create_compute_pipeline_hlsl(engine, path, input, input_size, pragmas);
                break;
            }
            case NONE:
            {
                mt_log_error("Missing shader entry point: \"%s\"", path);
                break;
            }
        }

        mt_array_free(engine->alloc, pragmas);

        return pipeline;
    }

    return NULL;
}
