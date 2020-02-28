static void graph_bake(MtRenderGraph *graph);
static void graph_unbake(MtRenderGraph *graph);

static inline bool find_in_array(uint32_t *array, uint32_t to_find)
{
    for (uint32_t i = 0; i < mt_array_size(array); ++i)
    {
        if (array[i] == to_find)
        {
            return true;
        }
    }

    return false;
}

static bool default_color_clearer(uint32_t render_target_index, MtClearColorValue *color)
{
    if (color)
    {
        color->float32[0] = 0.0f;
        color->float32[1] = 0.0f;
        color->float32[2] = 0.0f;
        color->float32[3] = 1.0f;
    }
    return true;
}

static bool default_depth_stencil_clearer(MtClearDepthStencilValue *depth_stencil)
{
    if (depth_stencil)
    {
        depth_stencil->depth = 1.0f;
        depth_stencil->stencil = 0;
    }
    return true;
}

static void
add_group(MtRenderGraph *graph, MtQueueType queue_type, uint32_t *pass_indices, bool last)
{
    ExecutionGroup group = {0};
    group.pass_indices = pass_indices;
    group.queue_type = queue_type;

    for (uint32_t i = 0; i < graph->frame_count; ++i)
    {
        allocate_cmd_buffers(graph->dev, group.queue_type, 1, &group.frames[i].cmd_buffer);

        VkFenceCreateInfo fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VK_CHECK(
            vkCreateFence(graph->dev->device, &fence_create_info, NULL, &group.frames[i].fence));

        VkSemaphoreCreateInfo semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VK_CHECK(vkCreateSemaphore(
            graph->dev->device,
            &semaphore_create_info,
            NULL,
            &group.frames[i].execution_finished_semaphore));

        if (last && graph->swapchain)
        {
            mt_array_push(
                graph->dev->alloc,
                group.frames[i].wait_semaphores,
                graph->image_available_semaphores[i]);
            mt_array_push(
                graph->dev->alloc,
                group.frames[i].wait_stages,
                MT_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT);
        }
    }

    if (mt_array_size(graph->execution_groups) > 0)
    {
        ExecutionGroup *prev_group = mt_array_last(graph->execution_groups);
        for (uint32_t i = 0; i < graph->frame_count; ++i)
        {
            VkPipelineStageFlags wait_stages = 0;
            for (uint32_t *j = group.pass_indices;
                 j != group.pass_indices + mt_array_size(group.pass_indices);
                 ++j)
            {
                wait_stages |= graph->passes[*j].stage;
            }
            mt_array_push(
                graph->dev->alloc,
                group.frames[i].wait_semaphores,
                prev_group->frames[i].execution_finished_semaphore);
            mt_array_push(graph->dev->alloc, group.frames[i].wait_stages, wait_stages);
        }
    }

    mt_array_push(graph->dev->alloc, graph->execution_groups, group);
}

static void destroy_group(MtRenderGraph *graph, ExecutionGroup *group)
{
    for (uint32_t i = 0; i < graph->frame_count; ++i)
    {
        free_cmd_buffers(graph->dev, group->queue_type, 1, &group->frames[i].cmd_buffer);
        vkDestroyFence(graph->dev->device, group->frames[i].fence, NULL);
        vkDestroySemaphore(graph->dev->device, group->frames[i].execution_finished_semaphore, NULL);

        mt_array_free(graph->dev->alloc, group->frames[i].wait_semaphores);
        mt_array_free(graph->dev->alloc, group->frames[i].wait_stages);
    }

    mt_array_free(graph->dev->alloc, group->pass_indices);
}

static MtRenderGraph *create_graph(MtDevice *dev, MtSwapchain *swapchain)
{
    MtRenderGraph *graph = mt_alloc(dev->alloc, sizeof(*graph));
    memset(graph, 0, sizeof(*graph));
    graph->dev = dev;
    graph->swapchain = swapchain;

    if (graph->swapchain)
    {
        graph->frame_count = FRAMES_IN_FLIGHT;
        for (uint32_t i = 0; i < graph->frame_count; ++i)
        {
            VkSemaphoreCreateInfo semaphore_create_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

            VK_CHECK(vkCreateSemaphore(
                dev->device, &semaphore_create_info, NULL, &graph->image_available_semaphores[i]));
        }
    }
    else
    {
        graph->frame_count = 1;
    }

    return graph;
}

static void destroy_graph(MtRenderGraph *graph)
{
    graph_unbake(graph);

    for (uint32_t i = 0; i < graph->frame_count; ++i)
    {
        if (graph->image_available_semaphores[i] != NULL)
        {
            vkDestroySemaphore(graph->dev->device, graph->image_available_semaphores[i], NULL);
        }
    }

    mt_array_free(graph->dev->alloc, graph->buffer_barriers);
    mt_array_free(graph->dev->alloc, graph->image_barriers);
    mt_free(graph->dev->alloc, graph);
}

static void graph_set_builder(MtRenderGraph *graph, MtRenderGraphBuilder builder)
{
    graph->graph_builder = builder;
}

static void graph_set_user_data(MtRenderGraph *graph, void *user_data)
{
    graph->user_data = user_data;
}

static void create_pass_renderpass(MtRenderGraph *graph, MtRenderGraphPass *pass);

static void graph_bake(MtRenderGraph *graph)
{
    graph->baked = true;

    //
    // Create stuff
    //
    mt_array_reserve(graph->dev->alloc, graph->passes, 32);
    mt_array_reserve(graph->dev->alloc, graph->resources, 64);

    mt_hash_init(&graph->pass_indices, mt_array_capacity(graph->passes), graph->dev->alloc);
    mt_hash_init(&graph->resource_indices, mt_array_capacity(graph->resources), graph->dev->alloc);

    //
    // Run the builder
    //
    assert(graph->graph_builder);
    graph->graph_builder(graph, graph->user_data);

    //
    // Add next & prev
    //

    for (uint32_t i = 0; i < mt_array_size(graph->passes); ++i)
    {
        if (i > 0)
        {
            graph->passes[i].prev = &graph->passes[i - 1];
        }

        if (i + 1 < mt_array_size(graph->passes))
        {
            graph->passes[i].next = &graph->passes[i + 1];
        }
    }

    //
    // Create execution groups
    //

    uint32_t prev_pass_index = 0;

    uint32_t *pass_indices = NULL;
    mt_array_push(graph->dev->alloc, pass_indices, prev_pass_index);

    for (uint32_t i = 1; i < mt_array_size(graph->passes); ++i)
    {
        MtRenderGraphPass *pass = &graph->passes[i];
        MtRenderGraphPass *prev_pass = &graph->passes[prev_pass_index];
        if (prev_pass->stage != pass->stage)
        {
            // Consolidate group
            add_group(graph, prev_pass->queue_type, pass_indices, false);

            pass_indices = NULL;
        }

        mt_array_push(graph->dev->alloc, pass_indices, i);

        prev_pass_index = i;
    }

    MtRenderGraphPass *last_pass = mt_array_last(graph->passes);

    if (graph->swapchain != NULL)
    {
        assert(last_pass->queue_type == MT_QUEUE_GRAPHICS);
        last_pass->present = true;
    }

    add_group(graph, last_pass->queue_type, pass_indices, true);

    //
    // Create resources
    //
    for (GraphResource *resource = graph->resources;
         resource != graph->resources + mt_array_size(graph->resources);
         ++resource)
    {
        switch (resource->type)
        {
            case GRAPH_RESOURCE_IMAGE: {
                for (uint32_t *pass_index = resource->written_in_passes;
                     pass_index !=
                     resource->written_in_passes + mt_array_size(resource->written_in_passes);
                     ++pass_index)
                {
                    MtRenderGraphPass *pass = &graph->passes[*pass_index];
                    if (pass->present)
                    {
                        resource->image_info.width = graph->swapchain->extent.width;
                        resource->image_info.height = graph->swapchain->extent.height;
                    }
                }

                resource->image = mt_render.create_image(graph->dev, &resource->image_info);
                break;
            }
            case GRAPH_RESOURCE_BUFFER: {
                resource->buffer = mt_render.create_buffer(graph->dev, &resource->buffer_info);
                break;
            }
            case GRAPH_RESOURCE_EXTERNAL_BUFFER: break;
        }
    }

    //
    // Create render passes
    //

    for (MtRenderGraphPass *pass = graph->passes;
         pass != graph->passes + mt_array_size(graph->passes);
         ++pass)
    {
        if (pass->queue_type == MT_QUEUE_GRAPHICS)
        {
            create_pass_renderpass(graph, pass);
        }
    }

    /* mt_log_debug( */
    /*     "Baked render graph with %lu execution groups", mt_array_size(graph->execution_groups));
     */
}

static void graph_unbake(MtRenderGraph *graph)
{
    graph->baked = false;

    //
    // Destroy the execution groups
    //
    for (uint32_t i = 0; i < mt_array_size(graph->execution_groups); ++i)
    {
        ExecutionGroup *group = &graph->execution_groups[i];
        destroy_group(graph, group);
    }
    mt_array_free(graph->dev->alloc, graph->execution_groups);
    graph->execution_groups = NULL;

    //
    // Destroy passes
    //
    for (MtRenderGraphPass *pass = graph->passes;
         pass != graph->passes + mt_array_size(graph->passes);
         ++pass)
    {
        if (pass->queue_type == MT_QUEUE_GRAPHICS)
        {
            device_wait_idle(graph->dev);
            vkDestroyRenderPass(graph->dev->device, pass->render_pass.renderpass, NULL);
            pass->render_pass.renderpass = VK_NULL_HANDLE;

            for (uint32_t i = 0; i < mt_array_size(pass->framebuffers); i++)
            {
                vkDestroyFramebuffer(graph->dev->device, pass->framebuffers[i], NULL);
            }

            mt_array_free(graph->dev->alloc, pass->framebuffers);
        }

        mt_array_free(graph->dev->alloc, pass->color_outputs);
        mt_array_free(graph->dev->alloc, pass->image_transfer_outputs);
        mt_array_free(graph->dev->alloc, pass->buffer_writes);

        mt_array_free(graph->dev->alloc, pass->image_transfer_inputs);
        mt_array_free(graph->dev->alloc, pass->image_sampled_inputs);
        mt_array_free(graph->dev->alloc, pass->buffer_reads);
    }

    mt_array_free(graph->dev->alloc, graph->passes);
    graph->passes = NULL;

    //
    // Destory the resources
    //
    for (GraphResource *resource = graph->resources;
         resource != graph->resources + mt_array_size(graph->resources);
         ++resource)
    {
        switch (resource->type)
        {
            case GRAPH_RESOURCE_IMAGE: {
                if (resource->image)
                {
                    // TODO: I wonder why I need this if...
                    mt_render.destroy_image(graph->dev, resource->image);
                }
                break;
            }
            case GRAPH_RESOURCE_BUFFER: {
                mt_render.destroy_buffer(graph->dev, resource->buffer);
                break;
            }
            case GRAPH_RESOURCE_EXTERNAL_BUFFER: break;
        }
        mt_array_free(graph->dev->alloc, resource->read_in_passes);
        mt_array_free(graph->dev->alloc, resource->written_in_passes);
    }

    mt_array_free(graph->dev->alloc, graph->resources);
    graph->resources = NULL;

    //
    // Destroy hashmaps
    //

    mt_hash_destroy(&graph->pass_indices);
    mt_hash_destroy(&graph->resource_indices);
}

static void graph_on_resize(MtRenderGraph *graph)
{
    graph->framebuffer_resized = true;
}

static void graph_wait_all(MtRenderGraph *graph)
{
    for (ExecutionGroup *group = graph->execution_groups;
         group != graph->execution_groups + mt_array_size(graph->execution_groups);
         ++group)
    {
        vkWaitForFences(
            graph->dev->device, 1, &group->frames[graph->current_frame].fence, VK_TRUE, UINT64_MAX);
    }
}

static void graph_execute(MtRenderGraph *graph)
{
    graph->current_frame = (graph->current_frame + 1) % graph->frame_count;

    if (graph->framebuffer_resized || !graph->baked)
    {
        graph->framebuffer_resized = false;

        if (graph->baked)
        {
            graph_unbake(graph);
        }

        graph_bake(graph);
    }

    if (graph->swapchain)
    {
        ExecutionGroup *group = mt_array_last(graph->execution_groups);

        vkWaitForFences(
            graph->dev->device, 1, &group->frames[graph->current_frame].fence, VK_TRUE, UINT64_MAX);

        VkResult res;
        while (1)
        {
            res = vkAcquireNextImageKHR(
                graph->dev->device,
                graph->swapchain->swapchain,
                UINT64_MAX,
                graph->image_available_semaphores[graph->current_frame],
                VK_NULL_HANDLE,
                &graph->swapchain->current_image_index);

            if (res != VK_ERROR_OUT_OF_DATE_KHR)
            {
                break;
            }

            swapchain_destroy_resizables(graph->swapchain);
            swapchain_create_resizables(graph->swapchain);

            graph->framebuffer_resized = true;
            graph_execute(graph);
            return;
        }

        VK_CHECK(res);
    }

    for (ExecutionGroup *group = graph->execution_groups;
         group != graph->execution_groups + mt_array_size(graph->execution_groups);
         ++group)
    {
        vkWaitForFences(
            graph->dev->device, 1, &group->frames[graph->current_frame].fence, VK_TRUE, UINT64_MAX);

        MtCmdBuffer *cb = group->frames[graph->current_frame].cmd_buffer;

        begin_cmd_buffer(cb);

        for (uint32_t *pass_index = group->pass_indices;
             pass_index != group->pass_indices + mt_array_size(group->pass_indices);
             ++pass_index)
        {
            MtRenderGraphPass *pass = &graph->passes[*pass_index];

            //
            // Apply some barriers
            //

            VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkPipelineStageFlags dst_stage_mask = pass->stage;
            if (pass->prev) src_stage_mask = pass->prev->stage;

            mt_array_set_size(graph->buffer_barriers, 0);
            mt_array_set_size(graph->image_barriers, 0);

            for (uint32_t *res = pass->image_transfer_outputs;
                 res != pass->image_transfer_outputs + mt_array_size(pass->image_transfer_outputs);
                 ++res)
            {
                MtImage *image = graph->resources[*res].image;

                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .image = image->image,
                    .subresourceRange =
                        {
                            .aspectMask = image->aspect,
                            .baseMipLevel = 0,
                            .levelCount = image->mip_count,
                            .baseArrayLayer = 0,
                            .layerCount = image->layer_count,
                        },
                };
                mt_array_push(graph->dev->alloc, graph->image_barriers, barrier);
            }

            for (uint32_t *res = pass->buffer_writes;
                 res != pass->buffer_writes + mt_array_size(pass->buffer_writes);
                 ++res)
            {
                VkBufferMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .srcAccessMask = 0,
                    .dstAccessMask = 0,
                    .buffer = graph->resources[*res].buffer->buffer,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
                };

                if (src_stage_mask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
                {
                    barrier.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT;
                }

                if (dst_stage_mask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
                {
                    barrier.srcAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
                }

                mt_array_push(graph->dev->alloc, graph->buffer_barriers, barrier);
            }

            vkCmdPipelineBarrier(
                cb->cmd_buffer,
                src_stage_mask,
                dst_stage_mask,
                0,
                0,
                NULL,
                (uint32_t)mt_array_size(graph->buffer_barriers),
                graph->buffer_barriers,
                (uint32_t)mt_array_size(graph->image_barriers),
                graph->image_barriers);

            //
            // Begin render pass (if applicable)
            //

            if (pass->queue_type == MT_QUEUE_GRAPHICS)
            {
                if (pass->present)
                {
                    pass->render_pass.current_framebuffer =
                        pass->framebuffers[graph->swapchain->current_image_index];
                }
                else
                {
                    pass->render_pass.current_framebuffer = pass->framebuffers[0];
                }

                cmd_begin_render_pass(cb, pass);
            }

            //
            // Record the commands
            //

            if (pass->builder)
            {
                pass->builder(graph, cb, graph->user_data);
            }

            //
            // End render pass (if applicable)
            //

            if (pass->queue_type == MT_QUEUE_GRAPHICS)
            {
                cmd_end_render_pass(cb);
            }

            //
            // Apply more barriers
            //

            src_stage_mask = pass->stage;
            dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            if (pass->next) dst_stage_mask = pass->next->stage;

            mt_array_set_size(graph->buffer_barriers, 0);
            mt_array_set_size(graph->image_barriers, 0);

            for (uint32_t *res = pass->image_transfer_outputs;
                 res != pass->image_transfer_outputs + mt_array_size(pass->image_transfer_outputs);
                 ++res)
            {
                MtImage *image = graph->resources[*res].image;

                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .image = image->image,
                    .subresourceRange =
                        {
                            .aspectMask = image->aspect,
                            .baseMipLevel = 0,
                            .levelCount = image->mip_count,
                            .baseArrayLayer = 0,
                            .layerCount = image->layer_count,
                        },
                };
                mt_array_push(graph->dev->alloc, graph->image_barriers, barrier);
            }

            for (uint32_t *res = pass->buffer_writes;
                 res != pass->buffer_writes + mt_array_size(pass->buffer_writes);
                 ++res)
            {
                VkBufferMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .srcAccessMask = 0,
                    .dstAccessMask = 0,
                    .buffer = graph->resources[*res].buffer->buffer,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
                };

                if (src_stage_mask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
                {
                    barrier.srcAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
                }

                if (dst_stage_mask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
                {
                    barrier.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT;
                }

                mt_array_push(graph->dev->alloc, graph->buffer_barriers, barrier);
            }

            vkCmdPipelineBarrier(
                cb->cmd_buffer,
                src_stage_mask,
                dst_stage_mask,
                0,
                0,
                NULL,
                (uint32_t)mt_array_size(graph->buffer_barriers),
                graph->buffer_barriers,
                (uint32_t)mt_array_size(graph->image_barriers),
                graph->image_barriers);
        }

        end_cmd_buffer(cb);

        VK_CHECK(vkResetFences(graph->dev->device, 1, &group->frames[graph->current_frame].fence));

        uint32_t signal_semaphore_count = 1;
        VkSemaphore *signal_semaphores =
            &group->frames[graph->current_frame].execution_finished_semaphore;

        if (group == mt_array_last(graph->execution_groups) && graph->swapchain == NULL)
        {
            signal_semaphore_count = 0;
            signal_semaphores = NULL;
        }

        submit_cmd(
            graph->dev,
            &(SubmitInfo){
                .cmd_buffer = cb,
                .wait_semaphore_count =
                    mt_array_size(group->frames[graph->current_frame].wait_semaphores),
                .wait_semaphores = group->frames[graph->current_frame].wait_semaphores,
                .wait_stages = group->frames[graph->current_frame].wait_stages,
                .signal_semaphore_count = signal_semaphore_count,
                .signal_semaphores = signal_semaphores,
                .fence = group->frames[graph->current_frame].fence,
            });
    }

    if (graph->swapchain)
    {
        ExecutionGroup *group = mt_array_last(graph->execution_groups);

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &group->frames[graph->current_frame].execution_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &graph->swapchain->swapchain,
            .pImageIndices = &graph->swapchain->current_image_index,
        };

        VkResult res = vkQueuePresentKHR(graph->swapchain->present_queue, &present_info);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
            swapchain_destroy_resizables(graph->swapchain);
            swapchain_create_resizables(graph->swapchain);

            graph->framebuffer_resized = true;
        }
        else
        {
            VK_CHECK(res);
        }

        swapchain_end_frame(graph->swapchain);
    }
}

static uint32_t add_graph_resource(MtRenderGraph *graph, const char *name, GraphResourceType type)
{
    mt_array_add(graph->dev->alloc, graph->resources, 1);
    uintptr_t index = mt_array_last(graph->resources) - graph->resources;
    GraphResource *resource = &graph->resources[index];
    memset(resource, 0, sizeof(*resource));

    resource->type = type;
    resource->index = (uint32_t)index;

    mt_hash_set_uint(&graph->resource_indices, mt_hash_str(name), index);
    return (uint32_t)index;
}

static void graph_add_image(MtRenderGraph *graph, const char *name, MtImageCreateInfo *info)
{
    uint32_t index = add_graph_resource(graph, name, GRAPH_RESOURCE_IMAGE);
    GraphResource *resource = &graph->resources[index];
    resource->image_info = *info;
}

static void graph_add_buffer(MtRenderGraph *graph, const char *name, MtBufferCreateInfo *info)
{
    uint32_t index = add_graph_resource(graph, name, GRAPH_RESOURCE_BUFFER);
    GraphResource *resource = &graph->resources[index];
    resource->buffer_info = *info;
}

static void graph_add_external_buffer(MtRenderGraph *graph, const char *name, MtBuffer *buffer)
{
    uint32_t index = add_graph_resource(graph, name, GRAPH_RESOURCE_EXTERNAL_BUFFER);
    GraphResource *resource = &graph->resources[index];
    resource->buffer = buffer;
}

static MtImage *graph_get_image(MtRenderGraph *graph, const char *name)
{
    uintptr_t index = mt_hash_get_uint(&graph->resource_indices, mt_hash_str(name));
    assert(index != MT_HASH_NOT_FOUND);
    assert(graph->resources[index].type == GRAPH_RESOURCE_IMAGE);
    return graph->resources[index].image;
}

static MtImage *graph_consume_image(MtRenderGraph *graph, const char *name)
{
    uintptr_t index = mt_hash_get_uint(&graph->resource_indices, mt_hash_str(name));
    assert(index != MT_HASH_NOT_FOUND);
    assert(graph->resources[index].type == GRAPH_RESOURCE_IMAGE);
    MtImage *image = graph->resources[index].image;
    graph->resources[index].image = NULL;
    return image;
}

static MtBuffer *graph_get_buffer(MtRenderGraph *graph, const char *name)
{
    uintptr_t index = mt_hash_get_uint(&graph->resource_indices, mt_hash_str(name));
    assert(index != MT_HASH_NOT_FOUND);
    assert(graph->resources[index].type == GRAPH_RESOURCE_BUFFER);
    return graph->resources[index].buffer;
}

static MtRenderGraphPass *
graph_add_pass(MtRenderGraph *graph, const char *name, MtPipelineStage stage)
{
    mt_array_add(graph->dev->alloc, graph->passes, 1);
    uint32_t index = mt_array_last(graph->passes) - graph->passes;
    MtRenderGraphPass *pass = mt_array_last(graph->passes);
    memset(pass, 0, sizeof(*pass));

    pass->color_clearer = default_color_clearer;
    pass->depth_stencil_clearer = default_depth_stencil_clearer;

    switch (stage)
    {
        case MT_PIPELINE_STAGE_FRAGMENT_SHADER:
            pass->stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case MT_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT:
            pass->stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case MT_PIPELINE_STAGE_ALL_GRAPHICS:
            pass->stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
            break;
        case MT_PIPELINE_STAGE_COMPUTE: pass->stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; break;
        case MT_PIPELINE_STAGE_TRANSFER: pass->stage = VK_PIPELINE_STAGE_TRANSFER_BIT; break;
    }

    switch (pass->stage)
    {
        case VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
        case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
        case VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT: pass->queue_type = MT_QUEUE_GRAPHICS; break;
        case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT: pass->queue_type = MT_QUEUE_COMPUTE; break;
        case VK_PIPELINE_STAGE_TRANSFER_BIT: pass->queue_type = MT_QUEUE_TRANSFER; break;
    }

    pass->graph = graph;
    pass->index = index;
    pass->name = name;
    pass->depth_output = UINT32_MAX;

    mt_hash_set_uint(&graph->pass_indices, mt_hash_str(name), index);
    return pass;
}

static void pass_set_color_clearer(MtRenderGraphPass *pass, MtRenderGraphColorClearer clearer)
{
    pass->color_clearer = clearer;
}

static void
pass_set_depth_stencil_clearer(MtRenderGraphPass *pass, MtRenderGraphDepthStencilClearer clearer)
{
    pass->depth_stencil_clearer = clearer;
}

static void pass_set_builder(MtRenderGraphPass *pass, MtRenderGraphPassBuilder builder)
{
    pass->builder = builder;
}

static void pass_read(MtRenderGraphPass *pass, MtRenderGraphPassRead type, const char *name)
{
    uintptr_t index = mt_hash_get_uint(&pass->graph->resource_indices, mt_hash_str(name));
    assert(index != MT_HASH_NOT_FOUND);
    GraphResource *resource = &pass->graph->resources[index];

    mt_array_push(pass->graph->dev->alloc, resource->read_in_passes, pass->index);

    switch (type)
    {
        case MT_PASS_READ_IMAGE_TRANSFER: {
            assert(resource->type == GRAPH_RESOURCE_IMAGE);
            resource->image_info.usage |= MT_IMAGE_USAGE_TRANSFER_SRC_BIT;
            mt_array_push(pass->graph->dev->alloc, pass->image_transfer_inputs, (uint32_t)index);
            break;
        }
        case MT_PASS_READ_IMAGE_SAMPLED: {
            assert(resource->type == GRAPH_RESOURCE_IMAGE);
            resource->image_info.usage |= MT_IMAGE_USAGE_SAMPLED_BIT;
            mt_array_push(pass->graph->dev->alloc, pass->image_sampled_inputs, (uint32_t)index);
            break;
        }
        case MT_PASS_READ_BUFFER: {
            assert(
                resource->type == GRAPH_RESOURCE_BUFFER ||
                resource->type == GRAPH_RESOURCE_EXTERNAL_BUFFER);
            mt_array_push(pass->graph->dev->alloc, pass->buffer_reads, (uint32_t)index);
            break;
        }
        default: assert(0);
    }
}

static void pass_write(MtRenderGraphPass *pass, MtRenderGraphPassWrite type, const char *name)
{
    uintptr_t index = mt_hash_get_uint(&pass->graph->resource_indices, mt_hash_str(name));
    assert(index != MT_HASH_NOT_FOUND);
    GraphResource *resource = &pass->graph->resources[index];

    mt_array_push(pass->graph->dev->alloc, resource->written_in_passes, pass->index);

    switch (type)
    {
        case MT_PASS_WRITE_COLOR_ATTACHMENT: {
            assert(resource->type == GRAPH_RESOURCE_IMAGE);

            resource->image_info.usage |=
                MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | MT_IMAGE_USAGE_SAMPLED_BIT;
            resource->image_info.aspect |= MT_IMAGE_ASPECT_COLOR_BIT;

            mt_array_push(pass->graph->dev->alloc, pass->color_outputs, (uint32_t)index);
            break;
        }
        case MT_PASS_WRITE_DEPTH_STENCIL_ATTACHMENT: {
            assert(resource->type == GRAPH_RESOURCE_IMAGE);

            resource->image_info.usage |= MT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            switch (resource->image_info.format)
            {
                case MT_FORMAT_D16_UNORM:
                case MT_FORMAT_D32_SFLOAT: {
                    resource->image_info.aspect = MT_IMAGE_ASPECT_DEPTH_BIT;
                    break;
                }

                case MT_FORMAT_D16_UNORM_S8_UINT:
                case MT_FORMAT_D24_UNORM_S8_UINT:
                case MT_FORMAT_D32_SFLOAT_S8_UINT: {
                    resource->image_info.aspect =
                        MT_IMAGE_ASPECT_DEPTH_BIT | MT_IMAGE_ASPECT_STENCIL_BIT;
                    break;
                }
                default: assert(0);
            }

            pass->depth_output = (uint32_t)index;
            break;
        }
        case MT_PASS_WRITE_BUFFER: {
            assert(
                resource->type == GRAPH_RESOURCE_BUFFER ||
                resource->type == GRAPH_RESOURCE_EXTERNAL_BUFFER);
            mt_array_push(pass->graph->dev->alloc, pass->buffer_writes, (uint32_t)index);
            break;
        }
        case MT_PASS_WRITE_IMAGE_TRANSFER: {
            assert(resource->type == GRAPH_RESOURCE_IMAGE);
            resource->image_info.usage |=
                MT_IMAGE_USAGE_TRANSFER_DST_BIT | MT_IMAGE_USAGE_SAMPLED_BIT;
            mt_array_push(pass->graph->dev->alloc, pass->image_transfer_outputs, (uint32_t)index);
            break;
        }
        default: assert(0);
    }
}

// Renderpass creation {{{
static void create_pass_renderpass(MtRenderGraph *graph, MtRenderGraphPass *pass)
{
    VkAttachmentDescription *rp_attachments = NULL;

    uint32_t *color_attachment_indices = NULL;
    uint32_t depth_attachment_index = UINT32_MAX;

    mt_array_free(graph->dev->alloc, pass->framebuffers);
    pass->framebuffers = NULL;

    if (graph->swapchain && pass->present)
    {
        //
        // Add swapchain attachments
        //

        mt_array_push(graph->dev->alloc, color_attachment_indices, mt_array_size(rp_attachments));

        VkAttachmentDescription color_attachment = {
            .format = graph->swapchain->swapchain_image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        if (pass->color_clearer((uint32_t)mt_array_size(rp_attachments), NULL))
        {
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        }

        mt_array_push(graph->dev->alloc, rp_attachments, color_attachment);

        mt_array_add(
            graph->dev->alloc, pass->framebuffers, graph->swapchain->swapchain_image_count);
    }
    else
    {
        mt_array_add(graph->dev->alloc, pass->framebuffers, 1);
    }

    //
    // Add the graph attachments
    //

    VkImageView *fb_image_views = NULL;

    for (uint32_t i = 0; i < mt_array_size(pass->color_outputs); ++i)
    {
        mt_array_push(graph->dev->alloc, color_attachment_indices, mt_array_size(rp_attachments));

        GraphResource *resource = &graph->resources[pass->color_outputs[i]];
        assert(resource->type == GRAPH_RESOURCE_IMAGE);

        VkAttachmentDescription color_attachment = {
            .format = resource->image->format,
            .samples = resource->image->sample_count,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        if (pass->color_clearer((uint32_t)mt_array_size(rp_attachments), NULL))
        {
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        }

        if (pass->next && pass->next->queue_type == MT_QUEUE_TRANSFER)
        {
            // TODO: the pass that needs this might not be the immediate next one...
            if (find_in_array(pass->next->image_transfer_inputs, resource->index))
            {
                color_attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            }
        }

        mt_array_push(graph->dev->alloc, rp_attachments, color_attachment);
        mt_array_push(graph->dev->alloc, fb_image_views, resource->image->image_view);
    }

    if (pass->depth_output != UINT32_MAX)
    {
        depth_attachment_index = mt_array_size(rp_attachments);

        GraphResource *resource = &graph->resources[pass->depth_output];
        assert(resource->type == GRAPH_RESOURCE_IMAGE);

        VkAttachmentDescription depth_attachment = {
            .format = resource->image->format,
            .samples = resource->image->sample_count,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        };

        if (pass->depth_stencil_clearer(NULL))
        {
            depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        }

        mt_array_push(graph->dev->alloc, rp_attachments, depth_attachment);
        mt_array_push(graph->dev->alloc, fb_image_views, resource->image->image_view);
    }

    //
    // Figure out the attachment references
    //

    VkAttachmentReference *color_attachment_refs = NULL;
    for (uint32_t i = 0; i < mt_array_size(color_attachment_indices); ++i)
    {
        VkAttachmentReference ref = {
            .attachment = color_attachment_indices[i],
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        mt_array_push(graph->dev->alloc, color_attachment_refs, ref);
    }

    VkAttachmentReference depth_attachment_ref = {
        .attachment = depth_attachment_index,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    if (mt_array_size(color_attachment_refs) > 0)
    {
        subpass.colorAttachmentCount = mt_array_size(color_attachment_refs);
        subpass.pColorAttachments = color_attachment_refs;
        pass->render_pass.color_attachment_count = (uint32_t)mt_array_size(color_attachment_refs);
    }

    if (depth_attachment_index != UINT32_MAX)
    {
        subpass.pDepthStencilAttachment = &depth_attachment_ref;
        pass->render_pass.has_depth_attachment = true;
    }

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    //
    // Create the render pass
    //

    VkRenderPassCreateInfo renderpass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = (uint32_t)mt_array_size(rp_attachments),
        .pAttachments = rp_attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VK_CHECK(vkCreateRenderPass(
        graph->dev->device, &renderpass_create_info, NULL, &pass->render_pass.renderpass));

    pass->render_pass.hash = vulkan_hash_render_pass(&renderpass_create_info);

    //
    // Create framebuffers
    //
    //
    if (pass->present)
    {
        pass->render_pass.extent = graph->swapchain->extent;
    }
    else
    {
        if (pass->color_outputs)
        {
            pass->render_pass.extent.width = graph->resources[pass->color_outputs[0]].image->width;
            pass->render_pass.extent.height =
                graph->resources[pass->color_outputs[0]].image->height;
        }
        else if (pass->depth_output != UINT32_MAX)
        {
            pass->render_pass.extent.width = graph->resources[pass->depth_output].image->width;
            pass->render_pass.extent.height = graph->resources[pass->depth_output].image->height;
        }
        else
        {
            assert(0);
        }
    }

    for (size_t i = 0; i < mt_array_size(pass->framebuffers); i++)
    {
        VkImageView *image_views = NULL;

        if (pass->present)
        {
            mt_array_push(
                graph->dev->alloc, image_views, graph->swapchain->swapchain_image_views[i]);
        }

        for (uint32_t j = 0; j < mt_array_size(fb_image_views); ++j)
        {
            mt_array_push(graph->dev->alloc, image_views, fb_image_views[j]);
        }

        VkFramebufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = pass->render_pass.renderpass,
            .attachmentCount = (uint32_t)mt_array_size(image_views),
            .pAttachments = image_views,
            .width = pass->render_pass.extent.width,
            .height = pass->render_pass.extent.height,
            .layers = 1,
        };

        VK_CHECK(
            vkCreateFramebuffer(graph->dev->device, &create_info, NULL, &pass->framebuffers[i]));

        mt_array_free(graph->dev->alloc, image_views);
    }

    mt_array_free(graph->dev->alloc, rp_attachments);
    mt_array_free(graph->dev->alloc, fb_image_views);
    mt_array_free(graph->dev->alloc, color_attachment_indices);
    mt_array_free(graph->dev->alloc, color_attachment_refs);
}
// }}}
