#include "spirv.h"
#include <spirv_cross_c.h>

typedef struct CombinedSetLayouts
{
    SetInfo sets[MAX_DESCRIPTOR_SETS];
    uint32_t set_count;
    uint64_t hash;
} CombinedSetLayouts;

static void
combined_set_layouts_init(CombinedSetLayouts *c, MtPipeline *pipeline, MtAllocator *alloc)
{
    memset(c, 0, sizeof(*c));

    for (Shader *shader = pipeline->shaders;
         shader != pipeline->shaders + mt_array_size(pipeline->shaders);
         ++shader)
    {
        c->set_count = MT_MAX(c->set_count, shader->set_count);
        for (uint32_t i = 0; i < shader->set_count; ++i)
        {
            SetInfo *shader_set = &shader->sets[i];

            SetInfo *set       = &c->sets[i];
            set->binding_count = MT_MAX(set->binding_count, shader_set->binding_count);
            for (uint32_t b = 0; b < shader_set->binding_count; ++b)
            {
                if (shader_set->bindings[b].descriptorCount == 0)
                {
                    continue;
                }
                assert(shader_set->bindings[b].descriptorCount == 1);

                set->bindings[b].binding = shader_set->bindings[b].binding;
                set->bindings[b].stageFlags |= shader_set->bindings[b].stageFlags;
                set->bindings[b].descriptorType  = shader_set->bindings[b].descriptorType;
                set->bindings[b].descriptorCount = shader_set->bindings[b].descriptorCount;
            }
        }
    }

    XXH64_state_t state = {0};

    for (SetInfo *set_info = c->sets; set_info != c->sets + c->set_count; ++set_info)
    {
        XXH64_update(
            &state, set_info->bindings, set_info->binding_count * sizeof(*set_info->bindings));
    }

    c->hash = (uint64_t)XXH64_digest(&state);
}

static void shader_init(MtDevice *dev, Shader *shader, uint8_t *code, size_t code_size)
{
    memset(shader, 0, sizeof(*shader));

    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode    = (uint32_t *)code,
    };
    VK_CHECK(vkCreateShaderModule(dev->device, &create_info, NULL, &shader->mod));

    spvc_context context     = NULL;
    spvc_parsed_ir ir        = NULL;
    spvc_compiler compiler   = NULL;
    spvc_resources resources = NULL;

    spvc_result result;

    result = spvc_context_create(&context);
    assert(result == SPVC_SUCCESS);

    result = spvc_context_parse_spirv(context, (uint32_t *)code, code_size / sizeof(uint32_t), &ir);
    assert(result == SPVC_SUCCESS);

    result = spvc_context_create_compiler(
        context, SPVC_BACKEND_NONE, ir, SPVC_CAPTURE_MODE_COPY, &compiler);
    assert(result == SPVC_SUCCESS);

    result = spvc_compiler_create_shader_resources(compiler, &resources);
    assert(result == SPVC_SUCCESS);

    const spvc_reflected_resource *input_list = NULL;
    size_t input_count                        = 0;
    result                                    = spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &input_list, &input_count);
    assert(result == SPVC_SUCCESS);

    SpvExecutionModel execution_model = spvc_compiler_get_execution_model(compiler);
    switch (execution_model)
    {
        case SpvExecutionModelVertex: shader->stage = VK_SHADER_STAGE_VERTEX_BIT; break;
        case SpvExecutionModelFragment: shader->stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
        case SpvExecutionModelGLCompute: shader->stage = VK_SHADER_STAGE_COMPUTE_BIT; break;
        default: assert(0);
    }

    if (shader->stage == VK_SHADER_STAGE_VERTEX_BIT)
    {
        for (uint32_t i = 0; i < input_count; i++)
        {
            uint32_t location =
                spvc_compiler_get_decoration(compiler, input_list[i].id, SpvDecorationLocation);

            if (location != UINT32_MAX)
            {
                shader->vertex_attribute_count =
                    MT_MAX(location + 1, shader->vertex_attribute_count);
            }
        }

        for (uint32_t i = 0; i < input_count; i++)
        {
            uint32_t location =
                spvc_compiler_get_decoration(compiler, input_list[i].id, SpvDecorationLocation);
            if (location == UINT32_MAX)
                continue;

            spvc_type type       = spvc_compiler_get_type_handle(compiler, input_list[i].type_id);
            uint32_t type_width  = spvc_type_get_bit_width(type);
            uint32_t vector_size = spvc_type_get_vector_size(type);

            VertexAttribute *attrib = &shader->vertex_attributes[location];
            attrib->size            = (type_width * vector_size) / 8;

            switch (attrib->size)
            {
                case sizeof(float): attrib->format = VK_FORMAT_R32_SFLOAT; break;
                case sizeof(float) * 2: attrib->format = VK_FORMAT_R32G32_SFLOAT; break;
                case sizeof(float) * 3: attrib->format = VK_FORMAT_R32G32B32_SFLOAT; break;
                case sizeof(float) * 4: attrib->format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
                default: assert(0);
            }
        }
    }

    spvc_resource_type resource_types[3] = {
        SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
        SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
        SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
    };

    for (uint32_t r = 0; r < MT_LENGTH(resource_types); ++r)
    {
        const spvc_reflected_resource *resource_list = NULL;
        size_t resource_count                        = 0;
        spvc_resources_get_resource_list_for_type(
            resources, resource_types[r], &resource_list, &resource_count);

        for (uint32_t i = 0; i < resource_count; ++i)
        {
            VkDescriptorType descriptor_type = 0;
            switch (resource_types[r])
            {
                case SPVC_RESOURCE_TYPE_SAMPLED_IMAGE:
                    descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
                case SPVC_RESOURCE_TYPE_UNIFORM_BUFFER:
                    descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
                case SPVC_RESOURCE_TYPE_STORAGE_BUFFER:
                    descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                default: assert(0);
            }

            if (!spvc_compiler_has_decoration(
                    compiler, resource_list[i].id, SpvDecorationDescriptorSet))
            {
                continue;
            }

            if (!spvc_compiler_has_decoration(compiler, resource_list[i].id, SpvDecorationBinding))
            {
                continue;
            }

            uint32_t set = spvc_compiler_get_decoration(
                compiler, resource_list[i].id, SpvDecorationDescriptorSet);
            uint32_t binding =
                spvc_compiler_get_decoration(compiler, resource_list[i].id, SpvDecorationBinding);

            shader->set_count               = MT_MAX(set + 1, shader->set_count);
            shader->sets[set].binding_count = MT_MAX(binding + 1, shader->sets[set].binding_count);

            shader->sets[set].bindings[binding] = (VkDescriptorSetLayoutBinding){
                .binding         = binding,
                .descriptorType  = descriptor_type,
                .descriptorCount = 1,
                .stageFlags      = shader->stage,
            };
        }
    }

    spvc_context_destroy(context);
}

static void shader_destroy(MtDevice *dev, Shader *shader)
{
    device_wait_idle(dev);
    vkDestroyShaderModule(dev->device, shader->mod, NULL);
}

static PipelineLayout *
create_pipeline_layout(MtDevice *dev, CombinedSetLayouts *combined, VkPipelineBindPoint bind_point)
{
    PipelineLayout *l = mt_alloc(dev->alloc, sizeof(PipelineLayout));
    memset(l, 0, sizeof(*l));

    l->bind_point = bind_point;

    l->set_count = combined->set_count;
    memcpy(l->sets, combined->sets, sizeof(l->sets));

    VkDescriptorSetLayout *set_layouts = NULL;
    mt_array_add_zeroed(dev->alloc, l->pools, l->set_count);
    for (uint32_t i = 0; i < mt_array_size(l->pools); i++)
    {
        descriptor_pool_init(dev, &l->pools[i], l, i);
        mt_array_push(dev->alloc, set_layouts, l->pools[i].set_layout);
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = mt_array_size(set_layouts),
        .pSetLayouts            = set_layouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL,
    };

    VK_CHECK(vkCreatePipelineLayout(dev->device, &pipeline_layout_info, NULL, &l->layout));

    mt_array_free(dev->alloc, set_layouts);

    return l;
}

static void destroy_pipeline_layout(MtDevice *dev, PipelineLayout *l)
{
    device_wait_idle(dev);

    for (uint32_t i = 0; i < mt_array_size(l->pools); i++)
    {
        descriptor_pool_destroy(dev, &l->pools[i]);
    }
    mt_array_free(dev->alloc, l->pools);

    vkDestroyPipelineLayout(dev->device, l->layout, NULL);

    mt_free(dev->alloc, l);
}

static PipelineLayout *request_pipeline_layout(MtDevice *dev, MtPipeline *pipeline)
{
    CombinedSetLayouts combined;
    combined_set_layouts_init(&combined, pipeline, dev->alloc);
    uint64_t hash = combined.hash;

    PipelineLayout *layout = mt_hash_get_ptr(&dev->pipeline_layout_map, hash);
    if (layout)
    {
        return layout;
    }

    layout       = create_pipeline_layout(dev, &combined, pipeline->bind_point);
    layout->hash = hash;

    return mt_hash_set_ptr(&dev->pipeline_layout_map, hash, layout);
}

static void create_graphics_pipeline_instance(
    MtDevice *dev, PipelineInstance *instance, MtRenderPass *render_pass, MtPipeline *pipeline)
{
    instance->bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    instance->pipeline   = pipeline;

    MtGraphicsPipelineCreateInfo *options = &pipeline->create_info;

    VertexAttribute *attribs = NULL;
    uint32_t attrib_count    = 0;

    VkPipelineShaderStageCreateInfo *stages = NULL;
    mt_array_add(dev->alloc, stages, mt_array_size(pipeline->shaders));
    for (uint32_t i = 0; i < mt_array_size(pipeline->shaders); i++)
    {
        stages[i] = (VkPipelineShaderStageCreateInfo){
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = pipeline->shaders[i].stage,
            .module = pipeline->shaders[i].mod,
            .pName  = "main",
        };
        if (stages[i].stage == VK_SHADER_STAGE_VERTEX_BIT)
        {
            attribs      = pipeline->shaders[i].vertex_attributes;
            attrib_count = pipeline->shaders[i].vertex_attribute_count;
        }
    }

    // Defaults are OK
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    uint32_t vertex_stride = 0;
    for (uint32_t i = 0; i < attrib_count; i++)
    {
        vertex_stride += attribs[i].size;
    }

    VkVertexInputBindingDescription binding_description = {
        .binding   = 0,
        .stride    = vertex_stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription *attributes = NULL;
    if (attrib_count > 0)
    {
        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.pVertexBindingDescriptions    = &binding_description;

        mt_array_add(dev->alloc, attributes, attrib_count);

        uint32_t attrib_offset = 0;

        for (uint32_t i = 0; i < attrib_count; i++)
        {
            attributes[i] = (VkVertexInputAttributeDescription){
                .location = i,
                .binding  = 0,
                .format   = attribs[i].format,
                .offset   = attrib_offset,
            };
            attrib_offset += attribs[i].size;
        }
    }

    vertex_input_info.vertexAttributeDescriptionCount = attrib_count;
    vertex_input_info.pVertexAttributeDescriptions    = attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)render_pass->extent.width,
        .height   = (float)render_pass->extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = render_pass->extent,
    };
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = options->line_width,
        .cullMode                = cull_mode_to_vulkan(options->cull_mode),
        .frontFace               = front_face_to_vulkan(options->front_face),
        .depthBiasEnable         = options->depth_bias,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable   = VK_FALSE,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading      = 0.0f,     // Optional
        .pSampleMask           = NULL,     // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable      = VK_FALSE, // Optional
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = options->depth_test,
        .depthWriteEnable = options->depth_write,
        .depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_enabled = {
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_disabled = {
        .blendEnable         = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendAttachmentState blend_infos[8];
    if (options->blending)
    {
        for (uint32_t i = 0; i < MT_LENGTH(blend_infos); ++i)
        {
            blend_infos[i] = color_blend_attachment_enabled;
        }
    }
    else
    {
        for (uint32_t i = 0; i < MT_LENGTH(blend_infos); ++i)
        {
            blend_infos[i] = color_blend_attachment_disabled;
        }
    }

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = render_pass->color_attachment_count,
        .pAttachments    = blend_infos,
        .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    VkDynamicState dynamic_states[4] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = MT_LENGTH(dynamic_states),
        .pDynamicStates    = dynamic_states,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = mt_array_size(stages),
        .pStages             = stages,
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depth_stencil,
        .pColorBlendState    = &color_blending,
        .pDynamicState       = &dynamic_state,
        .layout              = instance->pipeline->layout->layout,
        .renderPass          = render_pass->renderpass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    VK_CHECK(vkCreateGraphicsPipelines(
        dev->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &instance->vk_pipeline));

    mt_array_free(dev->alloc, attributes);
    mt_array_free(dev->alloc, stages);
}

static void
create_compute_pipeline_instance(MtDevice *dev, PipelineInstance *instance, MtPipeline *pipeline)
{
    instance->bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    instance->pipeline   = pipeline;

    VkPipelineShaderStageCreateInfo shader_stage = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
        .module              = pipeline->shaders[0].mod,
        .pName               = "main",
        .pSpecializationInfo = NULL,
    };

    VkComputePipelineCreateInfo create_info = {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage              = shader_stage,
        .layout             = instance->pipeline->layout->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = 0,
    };

    VK_CHECK(vkCreateComputePipelines(
        dev->device, VK_NULL_HANDLE, 1, &create_info, NULL, &instance->vk_pipeline));
}

static PipelineInstance *
request_graphics_pipeline_instance(MtDevice *dev, MtPipeline *pipeline, MtRenderPass *render_pass)
{
    XXH64_state_t state = {0};
    XXH64_update(&state, &pipeline->hash, sizeof(pipeline->hash));
    XXH64_update(&state, &render_pass->hash, sizeof(render_pass->hash));
    uint64_t hash = (uint64_t)XXH64_digest(&state);

    PipelineInstance *instance = mt_hash_get_ptr(&pipeline->instances, hash);
    if (instance)
        return instance;

    instance       = mt_alloc(dev->alloc, sizeof(PipelineInstance));
    instance->hash = hash;
    create_graphics_pipeline_instance(dev, instance, render_pass, pipeline);
    return mt_hash_set_ptr(&pipeline->instances, instance->hash, instance);
}

static PipelineInstance *request_compute_pipeline_instance(MtDevice *dev, MtPipeline *pipeline)
{
    PipelineInstance *instance = mt_hash_get_ptr(&pipeline->instances, pipeline->hash);
    if (instance)
        return instance;

    instance       = mt_alloc(dev->alloc, sizeof(PipelineInstance));
    instance->hash = pipeline->hash;

    create_compute_pipeline_instance(dev, instance, pipeline);
    return mt_hash_set_ptr(&pipeline->instances, instance->hash, instance);
}

static void destroy_pipeline_instance(MtDevice *dev, PipelineInstance *instance)
{
    device_wait_idle(dev);
    vkDestroyPipeline(dev->device, instance->vk_pipeline, NULL);
    mt_free(dev->alloc, instance);
}

static MtPipeline *create_graphics_pipeline(
    MtDevice *dev,
    uint8_t *vertex_code,
    size_t vertex_code_size,
    uint8_t *fragment_code,
    size_t fragment_code_size,
    MtGraphicsPipelineCreateInfo *ci)
{
    assert(vertex_code);

    MtPipeline *pipeline = mt_alloc(dev->alloc, sizeof(MtPipeline));
    memset(pipeline, 0, sizeof(*pipeline));
    pipeline->bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

    if (ci->line_width == 0.0f || memcmp(&(uint32_t){0}, &ci->line_width, sizeof(uint32_t)) == 0)
    {
        ci->line_width = 1.0f;
    }

    pipeline->create_info = *ci;

    Shader vertex_shader;
    shader_init(dev, &vertex_shader, vertex_code, vertex_code_size);
    mt_array_push(dev->alloc, pipeline->shaders, vertex_shader);

    // Fragment shader is optional
    if (fragment_code)
    {
        Shader fragment_shader;
        shader_init(dev, &fragment_shader, fragment_code, fragment_code_size);
        mt_array_push(dev->alloc, pipeline->shaders, fragment_shader);
    }

    XXH64_state_t state = {0};

    XXH64_update(&state, vertex_code, vertex_code_size);
    if (fragment_code)
    {
        XXH64_update(&state, fragment_code, fragment_code_size);
    }

    XXH64_update(&state, &ci->blending, sizeof(ci->blending));
    XXH64_update(&state, &ci->depth_test, sizeof(ci->depth_test));
    XXH64_update(&state, &ci->depth_write, sizeof(ci->depth_write));
    XXH64_update(&state, &ci->depth_bias, sizeof(ci->depth_bias));
    XXH64_update(&state, &ci->cull_mode, sizeof(ci->cull_mode));
    XXH64_update(&state, &ci->front_face, sizeof(ci->front_face));
    XXH64_update(&state, &ci->line_width, sizeof(ci->line_width));

    pipeline->hash = (uint64_t)XXH64_digest(&state);

    pipeline->layout = request_pipeline_layout(dev, pipeline);
    pipeline->layout->ref_count++;

    mt_hash_init(&pipeline->instances, 5, dev->alloc);

    return pipeline;
}

static MtPipeline *create_compute_pipeline(MtDevice *dev, uint8_t *code, size_t code_size)
{
    MtPipeline *pipeline = mt_alloc(dev->alloc, sizeof(MtPipeline));
    memset(pipeline, 0, sizeof(*pipeline));
    pipeline->bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

    mt_array_add(dev->alloc, pipeline->shaders, 1);

    shader_init(dev, &pipeline->shaders[0], code, code_size);

    XXH64_state_t state = {0};
    XXH64_update(&state, code, code_size);
    pipeline->hash = (uint64_t)XXH64_digest(&state);

    pipeline->layout = request_pipeline_layout(dev, pipeline);
    pipeline->layout->ref_count++;

    mt_hash_init(&pipeline->instances, 5, dev->alloc);

    return pipeline;
}

static void destroy_pipeline(MtDevice *dev, MtPipeline *pipeline)
{
    for (uint32_t i = 0; i < pipeline->instances.size; i++)
    {
        if (pipeline->instances.keys[i] != MT_HASH_UNUSED)
        {
            destroy_pipeline_instance(dev, (PipelineInstance *)pipeline->instances.values[i]);
        }
    }
    mt_hash_destroy(&pipeline->instances);

    for (uint32_t i = 0; i < mt_array_size(pipeline->shaders); i++)
    {
        shader_destroy(dev, &pipeline->shaders[i]);
    }
    mt_array_free(dev->alloc, pipeline->shaders);

    for (uint32_t i = 0; i < mt_array_size(pipeline->layout->pools); i++)
    {
        descriptor_pool_reset(dev, &pipeline->layout->pools[i]);
    }

    pipeline->layout->ref_count--;
    if (pipeline->layout->ref_count == 0)
    {
        mt_hash_remove(&dev->pipeline_layout_map, pipeline->layout->hash);
        destroy_pipeline_layout(dev, pipeline->layout);
    }

    mt_free(dev->alloc, pipeline);
}
