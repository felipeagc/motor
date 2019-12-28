#include "spirv.h"
#include "spirv_reflect.h"

typedef struct CombinedSetLayouts {
    SetInfo *sets;
    uint64_t hash;
    VkPushConstantRange *push_constants;
} CombinedSetLayouts;

static void combined_set_layouts_init(
    CombinedSetLayouts *c, MtPipeline *pipeline, MtArena *arena) {
    memset(c, 0, sizeof(*c));

    uint32_t pc_count = 0;

    Shader *shader;
    mt_array_foreach(shader, pipeline->shaders) {
        pc_count += mt_array_size(shader->push_constants);
    }

    if (pc_count > 0) {
        mt_array_pushn_zeroed(arena, c->push_constants, pc_count);

        uint32_t pc_index = 0;
        Shader *shader;
        mt_array_foreach(shader, pipeline->shaders) {
            VkPushConstantRange *pc;
            mt_array_foreach(pc, shader->push_constants) {
                c->push_constants[pc_index++] = *pc;
            }
        }

        assert(pc_index == mt_array_size(c->push_constants));
    }

    uint32_t set_count = 0;
    mt_array_foreach(shader, pipeline->shaders) {
        SetInfo *set_info;
        mt_array_foreach(set_info, shader->sets) {
            set_count = (set_count > set_info->index + 1) ? set_count
                                                          : set_info->index + 1;
        }
    }

    if (set_count > 0) {
        mt_array_pushn_zeroed(arena, c->sets, set_count);

        Shader *shader;
        mt_array_foreach(shader, pipeline->shaders) {
            SetInfo *shader_set;
            mt_array_foreach(shader_set, shader->sets) {
                uint32_t i = shader_set->index;

                VkDescriptorSetLayoutBinding *sbinding;
                mt_array_foreach(sbinding, shader_set->bindings) {
                    SetInfo *set = &c->sets[i];

                    VkDescriptorSetLayoutBinding *binding = NULL;

                    VkDescriptorSetLayoutBinding *b;
                    mt_array_foreach(b, c->sets[i].bindings) {
                        if (b->binding == sbinding->binding) {
                            binding = b;
                            break;
                        }
                    }

                    if (!binding) {
                        binding = mt_array_push(
                            arena,
                            set->bindings,
                            (VkDescriptorSetLayoutBinding){});
                        binding->binding = sbinding->binding;
                    }

                    binding->stageFlags |= sbinding->stageFlags;
                    binding->descriptorType  = sbinding->descriptorType;
                    binding->descriptorCount = sbinding->descriptorCount;
                }
            }
        }
    }

    XXH64_state_t state = {0};

    SetInfo *set_info;
    mt_array_foreach(set_info, c->sets) {
        VkDescriptorSetLayoutBinding *binding;
        mt_array_foreach(binding, set_info->bindings) {
            XXH64_update(&state, binding, sizeof(*binding));
        }
    }

    VkPushConstantRange *pc;
    mt_array_foreach(pc, c->push_constants) {
        XXH64_update(&state, pc, sizeof(*pc));
    }

    c->hash = (uint64_t)XXH64_digest(&state);
}

static void
combined_set_layouts_destroy(CombinedSetLayouts *c, MtArena *arena) {
    for (SetInfo *info = c->sets; info != c->sets + mt_array_size(c->sets);
         info++) {
        mt_array_free(arena, info->bindings);
    }
    mt_array_free(arena, c->sets);
    mt_array_free(arena, c->push_constants);
}

static void
shader_init(MtDevice *dev, Shader *shader, uint8_t *code, size_t code_size) {
    memset(shader, 0, sizeof(*shader));

    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode    = (uint32_t *)code,
    };
    VK_CHECK(
        vkCreateShaderModule(dev->device, &create_info, NULL, &shader->mod));

    SpvReflectShaderModule reflect_mod;
    SpvReflectResult result =
        spvReflectCreateShaderModule(code_size, (uint32_t *)code, &reflect_mod);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    shader->stage = (VkShaderStageFlagBits)reflect_mod.shader_stage;

    assert(reflect_mod.entry_point_count == 1);

    if (reflect_mod.descriptor_set_count > 0) {
        mt_array_pushn_zeroed(
            dev->arena, shader->sets, reflect_mod.descriptor_set_count);

        for (uint32_t i = 0; i < reflect_mod.descriptor_set_count; i++) {
            SpvReflectDescriptorSet *rset = &reflect_mod.descriptor_sets[i];

            SetInfo *set = &shader->sets[i];
            set->index   = rset->set;
            mt_array_pushn_zeroed(
                dev->arena, set->bindings, rset->binding_count);

            for (uint32_t b = 0; b < mt_array_size(set->bindings); b++) {
                VkDescriptorSetLayoutBinding *binding = &set->bindings[b];
                binding->binding = rset->bindings[b]->binding;
                binding->descriptorType =
                    (VkDescriptorType)rset->bindings[b]->descriptor_type;
                binding->descriptorCount    = 1;
                binding->stageFlags         = shader->stage;
                binding->pImmutableSamplers = NULL;
            }
        }
    }

    if (reflect_mod.push_constant_block_count > 0) {
        // Push constants
        mt_array_pushn_zeroed(
            dev->arena,
            shader->push_constants,
            reflect_mod.push_constant_block_count);

        for (uint32_t i = 0; i < reflect_mod.push_constant_block_count; i++) {
            SpvReflectBlockVariable *pc = &reflect_mod.push_constant_blocks[i];

            shader->push_constants[i].stageFlags =
                (VkShaderStageFlagBits)reflect_mod.shader_stage;
            shader->push_constants[i].offset = pc->absolute_offset;
            shader->push_constants[i].size   = pc->size;
        }
    }

    spvReflectDestroyShaderModule(&reflect_mod);
}

static void shader_destroy(MtDevice *dev, Shader *shader) {
    VK_CHECK(vkDeviceWaitIdle(dev->device));

    vkDestroyShaderModule(dev->device, shader->mod, NULL);

    SetInfo *set_info;
    mt_array_foreach(set_info, shader->sets) {
        mt_array_free(dev->arena, set_info->bindings);
    }

    mt_array_free(dev->arena, shader->sets);
    mt_array_free(dev->arena, shader->push_constants);
}

static void pipeline_layout_init(
    MtDevice *dev,
    PipelineLayout *l,
    CombinedSetLayouts *combined,
    VkPipelineBindPoint bind_point) {
    memset(l, 0, sizeof(*l));

    l->bind_point = bind_point;

    mt_array_pushn(
        dev->arena, l->push_constants, mt_array_size(combined->push_constants));
    memcpy(
        l->push_constants,
        combined->push_constants,
        mt_array_size(combined->push_constants) *
            sizeof(*combined->push_constants));

    mt_array_pushn_zeroed(dev->arena, l->sets, mt_array_size(combined->sets));

    SetInfo *set;
    for (uint32_t i = 0; i < mt_array_size(l->sets); i++) {
        SetInfo *cset = &combined->sets[i];
        mt_array_pushn(
            dev->arena, l->sets[i].bindings, mt_array_size(cset->bindings));
        memcpy(
            l->sets[i].bindings,
            cset->bindings,
            sizeof(*cset->bindings) * mt_array_size(cset->bindings));
    }

    VkDescriptorSetLayout *set_layouts = NULL;
    mt_array_pushn_zeroed(dev->arena, l->pools, mt_array_size(l->sets));
    for (uint32_t i = 0; i < mt_array_size(l->pools); i++) {
        descriptor_pool_init(dev, &l->pools[i], l, i);
        mt_array_push(dev->arena, set_layouts, l->pools[i].set_layout);
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = mt_array_size(set_layouts),
        .pSetLayouts            = set_layouts,
        .pushConstantRangeCount = mt_array_size(l->push_constants),
        .pPushConstantRanges    = l->push_constants,
    };

    VK_CHECK(vkCreatePipelineLayout(
        dev->device, &pipeline_layout_info, NULL, &l->layout));

    mt_array_free(dev->arena, set_layouts);
}

static void pipeline_layout_destroy(MtDevice *dev, PipelineLayout *l) {
    VK_CHECK(vkDeviceWaitIdle(dev->device));

    for (uint32_t i = 0; i < mt_array_size(l->pools); i++) {
        descriptor_pool_destroy(dev, &l->pools[i]);
    }
    mt_array_free(dev->arena, l->pools);

    vkDestroyPipelineLayout(dev->device, l->layout, NULL);

    SetInfo *set;
    mt_array_foreach(set, l->sets) { mt_array_free(dev->arena, set->bindings); }
    mt_array_free(dev->arena, l->sets);
}

static void pipeline_init_graphics(
    MtDevice *dev,
    MtPipeline *pipeline,
    uint8_t *vertex_code,
    size_t vertex_code_size,
    uint8_t *fragment_code,
    size_t fragment_code_size,
    MtGraphicsPipelineCreateInfo *ci) {
    memset(pipeline, 0, sizeof(*pipeline));

    if (ci->line_width == 0.0f ||
        memcmp(&(uint32_t){0}, &ci->line_width, sizeof(uint32_t)) == 0) {
        ci->line_width = 1.0f;
    }

    pipeline->create_info = *ci;
    mt_array_pushn(dev->arena, pipeline->shaders, 2);

    shader_init(dev, &pipeline->shaders[0], vertex_code, vertex_code_size);
    shader_init(dev, &pipeline->shaders[1], fragment_code, fragment_code_size);

    XXH64_state_t state = {0};

    XXH64_update(&state, vertex_code, vertex_code_size);
    XXH64_update(&state, fragment_code, fragment_code_size);

    XXH64_update(&state, &ci->blending, sizeof(ci->blending));
    XXH64_update(&state, &ci->depth_test, sizeof(ci->depth_test));
    XXH64_update(&state, &ci->depth_write, sizeof(ci->depth_write));
    XXH64_update(&state, &ci->depth_bias, sizeof(ci->depth_bias));
    XXH64_update(&state, &ci->cull_mode, sizeof(ci->cull_mode));
    XXH64_update(&state, &ci->front_face, sizeof(ci->front_face));
    XXH64_update(&state, &ci->line_width, sizeof(ci->line_width));

    XXH64_update(&state, &ci->vertex_stride, sizeof(ci->vertex_stride));
    for (uint32_t i = 0; i < ci->vertex_attribute_count; i++) {
        XXH64_update(
            &state,
            &ci->vertex_attributes[i],
            sizeof(ci->vertex_attributes[i]));
    }

    pipeline->hash = (uint64_t)XXH64_digest(&state);
}

static void pipeline_init_compute(
    MtDevice *dev, MtPipeline *pipeline, uint8_t *code, size_t code_size) {
    memset(pipeline, 0, sizeof(*pipeline));

    mt_array_pushn(dev->arena, pipeline->shaders, 1);

    shader_init(dev, &pipeline->shaders[0], code, code_size);

    XXH64_state_t state = {0};
    XXH64_update(&state, code, code_size);
    pipeline->hash = (uint64_t)XXH64_digest(&state);
}

static void pipeline_destroy(MtDevice *dev, MtPipeline *pipeline) {
    // TODO: destroy instances of this pipeline
    // TODO: deallocate descriptor sets related to this pipeline

    shader_destroy(dev, &pipeline->shaders[0]);
    shader_destroy(dev, &pipeline->shaders[1]);
    mt_array_free(dev->arena, pipeline->shaders);
}

static PipelineLayout *
request_pipeline_layout(MtDevice *dev, MtPipeline *pipeline) {
    CombinedSetLayouts combined;
    combined_set_layouts_init(&combined, pipeline, dev->arena);
    uint64_t hash = combined.hash;

    PipelineLayout *layout = mt_hash_get_ptr(&dev->pipeline_layout_map, hash);
    if (layout) {
        combined_set_layouts_destroy(&combined, dev->arena);
        return layout;
    }

    layout = mt_alloc(dev->arena, sizeof(PipelineLayout));
    pipeline_layout_init(dev, layout, &combined, pipeline->bind_point);

    combined_set_layouts_destroy(&combined, dev->arena);

    return mt_hash_set_ptr(&dev->pipeline_layout_map, hash, layout);
}

static void create_graphics_pipeline_instance(
    MtDevice *dev,
    PipelineInstance *instance,
    MtRenderPass *render_pass,
    MtPipeline *pipeline) {
    instance->bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    instance->layout     = request_pipeline_layout(dev, pipeline);

    MtGraphicsPipelineCreateInfo *options = &pipeline->create_info;

    VkPipelineShaderStageCreateInfo *stages = NULL;
    mt_array_pushn(dev->arena, stages, mt_array_size(pipeline->shaders));
    for (uint32_t i = 0; i < mt_array_size(pipeline->shaders); i++) {
        stages[i] = (VkPipelineShaderStageCreateInfo){
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = pipeline->shaders[i].stage,
            .module = pipeline->shaders[i].mod,
            .pName  = "main",
        };
    }

    // Defaults are OK
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkVertexInputBindingDescription binding_description = {
        .binding   = 0,
        .stride    = options->vertex_stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    if (options->vertex_stride > 0) {
        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.pVertexBindingDescriptions    = &binding_description;
    }

    VkVertexInputAttributeDescription *attributes = NULL;
    if (options->vertex_attribute_count > 0) {
        mt_array_pushn(dev->arena, attributes, options->vertex_attribute_count);

        for (uint32_t i = 0; i < options->vertex_attribute_count; i++) {
            attributes[i] = (VkVertexInputAttributeDescription){
                .location = i,
                .binding  = 0,
                .format =
                    format_to_vulkan(options->vertex_attributes[i].format),
                .offset = options->vertex_attributes[i].offset,
            };
        }

        vertex_input_info.vertexAttributeDescriptionCount =
            mt_array_size(attributes);
        vertex_input_info.pVertexAttributeDescriptions = attributes;
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
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
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
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
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable   = VK_FALSE,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading      = 1.0f,     // Optional
        .pSampleMask           = NULL,     // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable      = VK_FALSE, // Optional
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = options->depth_test,
        .depthWriteEnable = options->depth_write,
        .depthCompareOp   = VK_COMPARE_OP_LESS,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_enabled = {
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
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
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment_disabled,
        .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    if (options->blending) {
        color_blending.pAttachments = &color_blend_attachment_enabled;
    }

    VkDynamicState dynamic_states[4] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
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
        .layout              = instance->layout->layout,
        .renderPass          = render_pass->renderpass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    VK_CHECK(vkCreateGraphicsPipelines(
        dev->device,
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        NULL,
        &instance->pipeline));

    mt_array_free(dev->arena, attributes);
    mt_array_free(dev->arena, stages);
}

static void create_compute_pipeline_instance(
    MtDevice *dev, PipelineInstance *instance, MtPipeline *pipeline) {
    instance->bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    instance->layout     = request_pipeline_layout(dev, pipeline);

    VkPipelineShaderStageCreateInfo shader_stage = {
        .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
        .module              = pipeline->shaders[0].mod,
        .pName               = "main",
        .pSpecializationInfo = NULL,
    };

    VkComputePipelineCreateInfo create_info = {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage              = shader_stage,
        .layout             = instance->layout->layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = 0,
    };

    VK_CHECK(vkCreateComputePipelines(
        dev->device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        NULL,
        &instance->pipeline));
}

static PipelineInstance *request_graphics_pipeline_instance(
    MtDevice *dev, MtPipeline *pipeline, MtRenderPass *render_pass) {
    XXH64_state_t state = {0};
    XXH64_update(&state, &pipeline->hash, sizeof(pipeline->hash));
    XXH64_update(&state, &render_pass->hash, sizeof(render_pass->hash));
    uint64_t hash = (uint64_t)XXH64_digest(&state);

    PipelineInstance *instance = mt_hash_get_ptr(&dev->pipeline_map, hash);
    if (instance) return instance;

    instance = mt_alloc(dev->arena, sizeof(PipelineInstance));

    create_graphics_pipeline_instance(dev, instance, render_pass, pipeline);

    return mt_hash_set_ptr(&dev->pipeline_map, hash, instance);
}

static PipelineInstance *
request_compute_pipeline_instance(MtDevice *dev, MtPipeline *pipeline) {
    PipelineInstance *instance =
        mt_hash_get_ptr(&dev->pipeline_map, pipeline->hash);
    if (instance) return instance;

    instance = mt_alloc(dev->arena, sizeof(PipelineInstance));

    create_compute_pipeline_instance(dev, instance, pipeline);

    return mt_hash_set_ptr(&dev->pipeline_map, pipeline->hash, instance);
}
