static void set_viewport(
    MtCmdBuffer *cmd_buffer, float x, float y, float width, float height) {
    VkViewport viewport = {
        .width    = width,
        .height   = height,
        .x        = x,
        .y        = y,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd_buffer->cmd_buffer, 0, 1, &viewport);
}

static void set_scissor(
    MtCmdBuffer *cmd_buffer,
    int32_t x,
    int32_t y,
    uint32_t width,
    uint32_t height) {
    VkRect2D scissor = {
        .offset = {x, y},
        .extent = {width, height},
    };

    vkCmdSetScissor(cmd_buffer->cmd_buffer, 0, 1, &scissor);
}

static void bind_pipeline(MtCmdBuffer *cb, MtPipeline *pipeline) {
    switch (pipeline->bind_point) {
    case VK_PIPELINE_BIND_POINT_GRAPHICS: {
        cb->bound_pipeline = request_graphics_pipeline(
            cb->dev, pipeline, &cb->current_renderpass);

        vkCmdBindPipeline(
            cb->cmd_buffer,
            cb->bound_pipeline->bind_point,
            cb->bound_pipeline->pipeline);
    } break;

    case VK_PIPELINE_BIND_POINT_COMPUTE: {
        // TODO
        /* bound_pipeline = dev.request_compute_pipeline(program); */

        /* vkCmdBindPipeline( */
        /*     cmd_buffer, */
        /*     bound_pipeline.bind_point, */
        /*     bound_pipeline.pipeline); */
    } break;
    }
}

static void
begin_render_pass(MtCmdBuffer *cmd_buffer, MtRenderPass *render_pass) {
    cmd_buffer->current_renderpass = *render_pass;

    VkClearValue clear_values[2] = {0};
    clear_values[0].color        = (VkClearColorValue){0.0f, 0.0f, 0.0f, 1.0f};
    clear_values[1].depthStencil.depth   = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo render_pass_info = {
        .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass        = render_pass->renderpass,
        .framebuffer       = render_pass->current_framebuffer,
        .renderArea.offset = (VkOffset2D){0, 0},
        .renderArea.extent = render_pass->extent,
        .clearValueCount   = MT_LENGTH(clear_values),
        .pClearValues      = clear_values,
    };

    vkCmdBeginRenderPass(
        cmd_buffer->cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    set_viewport(
        cmd_buffer,
        0.0f,
        0.0f,
        (float)render_pass->extent.width,
        (float)render_pass->extent.height);
    set_scissor(
        cmd_buffer,
        0,
        0,
        render_pass->extent.width,
        render_pass->extent.height);
}

static void end_render_pass(MtCmdBuffer *cmd_buffer) {
    vkCmdEndRenderPass(cmd_buffer->cmd_buffer);
}

static void begin_cmd_buffer(MtCmdBuffer *cmd_buffer) {
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd_buffer->cmd_buffer, &begin_info));
}

static void end_cmd_buffer(MtCmdBuffer *cmd_buffer) {
    VK_CHECK(vkEndCommandBuffer(cmd_buffer->cmd_buffer));
}

static void set_uniform(
    MtCmdBuffer *cmd_buffer,
    uint32_t set,
    uint32_t binding,
    void *data,
    size_t size) {
    assert(MT_LENGTH(cmd_buffer->bound_descriptors) > set);
    assert(MT_LENGTH(cmd_buffer->bound_descriptors[set]) > binding);

    MtBuffer *buffer;
    size_t offset;
    void *memory = buffer_allocator_allocate(
        &cmd_buffer->dev->ubo_allocator, size, &buffer, &offset);
    assert(memory);
    assert(buffer);

    memcpy(memory, data, size);

    Descriptor descriptor;
    descriptor.buffer.buffer                    = buffer->buffer;
    descriptor.buffer.offset                    = offset;
    descriptor.buffer.range                     = size;
    cmd_buffer->bound_descriptors[set][binding] = descriptor;
}

static void bind_descriptor_set(MtCmdBuffer *cb, uint32_t set) {
    assert(cb->bound_pipeline);

    DSAllocator *set_allocator = cb->bound_pipeline->layout->set_allocator;

    uint32_t binding_count =
        mt_array_size(cb->bound_pipeline->layout->sets[set].bindings);

    VkDescriptorSet descriptor_set = ds_allocator_allocate(
        set_allocator, set, cb->bound_descriptors[set], binding_count);

    assert(descriptor_set);

    vkCmdBindDescriptorSets(
        cb->cmd_buffer,
        cb->bound_pipeline->bind_point,
        cb->bound_pipeline->layout->layout,
        set,
        1,
        &descriptor_set,
        0,
        NULL);
}

static void
draw(MtCmdBuffer *cmd_buffer, uint32_t vertex_count, uint32_t instance_count) {
    vkCmdDraw(cmd_buffer->cmd_buffer, vertex_count, instance_count, 0, 0);
}
