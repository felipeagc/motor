static void add_group(
    MtRenderGraph *graph,
    MtQueueType queue_type,
    MtPipelineStage stage,
    uint32_t *pass_indices,
    bool present)
{
    ExecutionGroup group = {0};
    group.pass_indices   = pass_indices;
    group.queue_type     = queue_type;
    group.stage          = stage;
    group.present_group  = present;

    for (uint32_t i = 0; i < graph->frame_count; ++i)
    {
        mt_render.allocate_cmd_buffers(
            graph->dev, group.queue_type, 1, &group.frames[i].cmd_buffer);
        group.frames[i].fence                        = mt_render.create_fence(graph->dev, true);
        group.frames[i].execution_finished_semaphore = mt_render.create_semaphore(graph->dev);

        if (group.present_group)
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
            mt_array_push(
                graph->dev->alloc,
                group.frames[i].wait_semaphores,
                prev_group->frames[i].execution_finished_semaphore);
            mt_array_push(graph->dev->alloc, group.frames[i].wait_stages, prev_group->stage);
        }
    }

    mt_array_push(graph->dev->alloc, graph->execution_groups, group);
}

static void destroy_group(MtRenderGraph *graph, ExecutionGroup *group)
{
    for (uint32_t i = 0; i < graph->frame_count; ++i)
    {
        mt_render.free_cmd_buffers(graph->dev, group->queue_type, 1, &group->frames[i].cmd_buffer);
        mt_render.destroy_fence(graph->dev, group->frames[i].fence);
        mt_render.destroy_semaphore(graph->dev, group->frames[i].execution_finished_semaphore);

        mt_array_free(graph->dev->alloc, group->frames[i].wait_semaphores);
        mt_array_free(graph->dev->alloc, group->frames[i].wait_stages);
    }

    mt_array_free(graph->dev->alloc, group->pass_indices);
}

static MtRenderGraph *create_graph(MtDevice *dev, MtSwapchain *swapchain, void *user_data)
{
    MtRenderGraph *graph = mt_alloc(dev->alloc, sizeof(*graph));
    memset(graph, 0, sizeof(*graph));
    graph->dev                   = dev;
    graph->swapchain             = swapchain;
    graph->user_data             = user_data;
    graph->backbuffer_pass_index = UINT32_MAX;

    if (graph->swapchain)
    {
        graph->frame_count = FRAMES_IN_FLIGHT;
        for (uint32_t i = 0; i < graph->frame_count; ++i)
        {
            graph->image_available_semaphores[i] = mt_render.create_semaphore(graph->dev);
        }
    }
    else
    {
        graph->frame_count = 1;
    }

    mt_array_reserve(dev->alloc, graph->passes, 32);
    mt_array_reserve(dev->alloc, graph->attachments, 64);

    mt_hash_init(&graph->pass_indices, mt_array_capacity(graph->passes), dev->alloc);
    mt_hash_init(&graph->attachment_indices, mt_array_capacity(graph->attachments), dev->alloc);

    return graph;
}

static void graph_bake(MtRenderGraph *graph);
static void graph_unbake(MtRenderGraph *graph);

static void graph_on_resize(MtRenderGraph *graph)
{
    graph_unbake(graph);
    graph_bake(graph);
}

static void destroy_graph(MtRenderGraph *graph)
{
    graph_unbake(graph);

    for (uint32_t i = 0; i < mt_array_size(graph->passes); ++i)
    {
        MtRenderGraphPass *pass = &graph->passes[i];
        mt_array_free(graph->dev->alloc, pass->inputs);
        mt_array_free(graph->dev->alloc, pass->color_outputs);
    }

    for (uint32_t i = 0; i < graph->frame_count; ++i)
    {
        if (graph->image_available_semaphores[i] != NULL)
        {
            mt_render.destroy_semaphore(graph->dev, graph->image_available_semaphores[i]);
        }
    }

    mt_hash_destroy(&graph->pass_indices);
    mt_hash_destroy(&graph->attachment_indices);
    mt_array_free(graph->dev->alloc, graph->passes);
    mt_array_free(graph->dev->alloc, graph->attachments);
    mt_array_free(graph->dev->alloc, graph->execution_groups);
    mt_free(graph->dev->alloc, graph);
}

static void graph_set_backbuffer(MtRenderGraph *graph, const char *name)
{
    graph->backbuffer_pass_index = mt_hash_get_uint(&graph->pass_indices, mt_hash_str(name));
}

static void topological_sort(MtRenderGraph *graph, uint32_t v, bool *visited, uint32_t *stack)
{
    MtAllocator *alloc      = graph->dev->alloc;
    MtRenderGraphPass *list = graph->passes;

    visited[v] = true;

    // Gather adjacents
    uint32_t *adj = NULL;
    for (uint32_t i = 0; i < mt_array_size(list[v].inputs); ++i)
    {
        uint32_t pass_index = graph->attachments[list[v].inputs[i]].pass_index;

        bool found = false;
        for (uint32_t j = 0; j < mt_array_size(adj); ++j)
        {
            if (adj[j] == pass_index)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            mt_array_push(alloc, adj, pass_index);
        }
    }

    for (uint32_t i = 0; i < mt_array_size(adj); ++i)
    {
        if (!visited[adj[i]])
        {
            topological_sort(graph, adj[i], visited, stack);
        }
    }

    mt_array_push(alloc, stack, v);
    mt_array_free(alloc, adj);
}

static void graph_bake(MtRenderGraph *graph)
{
    // Attachment image creation {{{
    for (uint32_t i = 0; i < mt_array_size(graph->attachments); i++)
    {
        GraphAttachment *attachment = &graph->attachments[i];

        assert(attachment->info.format != MT_FORMAT_UNDEFINED);

        uint32_t width  = attachment->info.width;
        uint32_t height = attachment->info.height;

        if (width == 0 && graph->swapchain)
        {
            width = graph->swapchain->swapchain_extent.width;
        }

        if (height == 0 && graph->swapchain)
        {
            height = graph->swapchain->swapchain_extent.height;
        }

        assert(width > 0);
        assert(height > 0);

        MtImageUsage usage;
        MtImageAspect aspect;

        switch (attachment->info.format)
        {
            case MT_FORMAT_UNDEFINED:
            {
                mt_log_fatal("Used invalid format for graph pass attachment");
                abort();
                break;
            }

            case MT_FORMAT_R8_UINT:
            case MT_FORMAT_R32_UINT:
            case MT_FORMAT_R8_UNORM:
            case MT_FORMAT_RG8_UNORM:
            case MT_FORMAT_RGB8_UNORM:
            case MT_FORMAT_RGBA8_UNORM:
            case MT_FORMAT_BGRA8_UNORM:
            case MT_FORMAT_BGRA8_SRGB:
            case MT_FORMAT_R32_SFLOAT:
            case MT_FORMAT_RG32_SFLOAT:
            case MT_FORMAT_RGB32_SFLOAT:
            case MT_FORMAT_RGBA32_SFLOAT:
            case MT_FORMAT_RG16_SFLOAT:
            case MT_FORMAT_RGBA16_SFLOAT:
            case MT_FORMAT_BC7_UNORM_BLOCK:
            case MT_FORMAT_BC7_SRGB_BLOCK:
            {
                aspect = MT_IMAGE_ASPECT_COLOR_BIT;
                usage  = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                break;
            }

            case MT_FORMAT_D16_UNORM:
            case MT_FORMAT_D32_SFLOAT:
            {
                aspect = MT_IMAGE_ASPECT_DEPTH_BIT;
                usage  = MT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                break;
            }

            case MT_FORMAT_D16_UNORM_S8_UINT:
            case MT_FORMAT_D24_UNORM_S8_UINT:
            case MT_FORMAT_D32_SFLOAT_S8_UINT:
            {
                aspect = MT_IMAGE_ASPECT_DEPTH_BIT | MT_IMAGE_ASPECT_STENCIL_BIT;
                usage  = MT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                break;
            }
        }

        attachment->image = mt_render.create_image(
            graph->dev,
            &(MtImageCreateInfo){
                .width        = width,
                .height       = height,
                .depth        = 1,
                .sample_count = attachment->info.sample_count,
                .mip_count    = attachment->info.mip_count,
                .layer_count  = attachment->info.layer_count,
                .format       = attachment->info.format,
                .aspect       = aspect,
                .usage        = usage,
            });
    }
    // }}}

    // RenderPass & framebuffer creation {{{
    for (MtRenderGraphPass *pass = graph->passes;
         pass != graph->passes + mt_array_size(graph->passes);
         ++pass)
    {
        VkAttachmentDescription *rp_attachments = NULL;

        uint32_t *color_attachment_indices = NULL;
        uint32_t depth_attachment_index    = UINT32_MAX;

        mt_array_free(graph->dev->alloc, pass->framebuffers);
        pass->framebuffers = NULL;

        if (graph->swapchain && graph->backbuffer_pass_index == pass->index)
        {
            //
            // Add swapchain attachments
            //

            mt_array_push(
                graph->dev->alloc, color_attachment_indices, mt_array_size(rp_attachments));

            VkAttachmentDescription color_attachment = {
                .format         = graph->swapchain->swapchain_image_format,
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            };

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
            mt_array_push(
                graph->dev->alloc, color_attachment_indices, mt_array_size(rp_attachments));

            GraphAttachment *graph_attachment = &graph->attachments[pass->color_outputs[i]];

            VkAttachmentDescription color_attachment = {
                .format         = graph_attachment->image->format,
                .samples        = graph_attachment->image->sample_count,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            mt_array_push(graph->dev->alloc, rp_attachments, color_attachment);
            mt_array_push(graph->dev->alloc, fb_image_views, graph_attachment->image->image_view);
        }

        if (pass->depth_output != UINT32_MAX)
        {
            depth_attachment_index = mt_array_size(rp_attachments);

            GraphAttachment *graph_attachment = &graph->attachments[pass->depth_output];

            VkAttachmentDescription depth_attachment = {
                .format         = graph_attachment->image->format,
                .samples        = graph_attachment->image->sample_count,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            };

            mt_array_push(graph->dev->alloc, rp_attachments, depth_attachment);
            mt_array_push(graph->dev->alloc, fb_image_views, graph_attachment->image->image_view);
        }

        //
        // Figure out the attachment references
        //

        VkAttachmentReference *color_attachment_refs = NULL;
        for (uint32_t i = 0; i < mt_array_size(color_attachment_indices); ++i)
        {
            VkAttachmentReference ref = {
                .attachment = color_attachment_indices[i],
                .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };
            mt_array_push(graph->dev->alloc, color_attachment_refs, ref);
        }

        VkAttachmentReference depth_attachment_ref = {
            .attachment = depth_attachment_index,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass = {0};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

        if (mt_array_size(color_attachment_refs) > 0)
        {
            subpass.colorAttachmentCount = mt_array_size(color_attachment_refs);
            subpass.pColorAttachments    = color_attachment_refs;
            pass->render_pass.color_attachment_count =
                (uint32_t)mt_array_size(color_attachment_refs);
        }

        if (depth_attachment_index != UINT32_MAX)
        {
            subpass.pDepthStencilAttachment        = &depth_attachment_ref;
            pass->render_pass.has_depth_attachment = true;
        }

        VkSubpassDependency dependency = {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        //
        // Create the render pass
        //

        VkRenderPassCreateInfo renderpass_create_info = {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = (uint32_t)mt_array_size(rp_attachments),
            .pAttachments    = rp_attachments,
            .subpassCount    = 1,
            .pSubpasses      = &subpass,
            .dependencyCount = 1,
            .pDependencies   = &dependency,
        };

        VK_CHECK(vkCreateRenderPass(
            graph->dev->device, &renderpass_create_info, NULL, &pass->render_pass.renderpass));

        pass->render_pass.hash = vulkan_hash_render_pass(&renderpass_create_info);

        //
        // Create framebuffers
        //
        //
        if (graph->swapchain && graph->backbuffer_pass_index == pass->index)
        {
            pass->render_pass.extent = graph->swapchain->swapchain_extent;
        }
        else
        {
            pass->render_pass.extent.width =
                graph->attachments[pass->color_outputs[0]].image->width;
            pass->render_pass.extent.height =
                graph->attachments[pass->color_outputs[0]].image->height;
        }

        for (size_t i = 0; i < mt_array_size(pass->framebuffers); i++)
        {
            VkImageView *image_views = NULL;

            if (graph->swapchain && graph->backbuffer_pass_index == pass->index)
            {
                mt_array_push(
                    graph->dev->alloc, image_views, graph->swapchain->swapchain_image_views[i]);
            }

            for (uint32_t j = 0; j < mt_array_size(fb_image_views); ++j)
            {
                mt_array_push(graph->dev->alloc, image_views, fb_image_views[j]);
            }

            VkFramebufferCreateInfo create_info = {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = pass->render_pass.renderpass,
                .attachmentCount = (uint32_t)mt_array_size(image_views),
                .pAttachments    = image_views,
                .width           = pass->render_pass.extent.width,
                .height          = pass->render_pass.extent.height,
                .layers          = 1,
            };

            VK_CHECK(vkCreateFramebuffer(
                graph->dev->device, &create_info, NULL, &pass->framebuffers[i]));

            mt_array_free(graph->dev->alloc, image_views);
        }

        mt_array_free(graph->dev->alloc, rp_attachments);
        mt_array_free(graph->dev->alloc, fb_image_views);
        mt_array_free(graph->dev->alloc, color_attachment_indices);
    }
    // }}}

    bool *visited = NULL;
    mt_array_add(graph->dev->alloc, visited, mt_array_size(graph->passes));
    for (uint32_t i = 0; i < mt_array_size(visited); i++)
    {
        visited[i] = false;
    }

    if (graph->ordered_passes)
    {
        mt_array_free(graph->dev->alloc, graph->ordered_passes);
        graph->ordered_passes = NULL;
    }
    mt_array_reserve(graph->dev->alloc, graph->ordered_passes, mt_array_size(graph->passes));

    topological_sort(graph, mt_array_size(graph->passes) - 1, visited, graph->ordered_passes);

    mt_array_free(graph->dev->alloc, visited);

    uint32_t last_pass_index = graph->ordered_passes[0];

    uint32_t *pass_indices = NULL;
    mt_array_push(graph->dev->alloc, pass_indices, last_pass_index);

    for (uint32_t i = 1; i < mt_array_size(graph->ordered_passes); ++i)
    {
        MtRenderGraphPass *pass      = &graph->passes[graph->ordered_passes[i]];
        MtRenderGraphPass *last_pass = &graph->passes[last_pass_index];
        if (last_pass->stage != pass->stage)
        {
            // Consolidate group
            add_group(graph, last_pass->queue_type, last_pass->stage, pass_indices, false);

            pass_indices = NULL;
        }

        mt_array_push(graph->dev->alloc, pass_indices, graph->ordered_passes[i]);

        last_pass_index = graph->ordered_passes[i];
    }

    MtRenderGraphPass *last_pass = &graph->passes[*mt_array_last(graph->ordered_passes)];

    bool present = (graph->swapchain != NULL);
    if (present)
    {
        assert(last_pass->queue_type == MT_QUEUE_GRAPHICS);
    }

    add_group(graph, last_pass->queue_type, last_pass->stage, pass_indices, present);

    mt_log("Baked render graph with %lu execution groups", mt_array_size(graph->execution_groups));
}

static void graph_unbake(MtRenderGraph *graph)
{
    for (uint32_t i = 0; i < mt_array_size(graph->passes); ++i)
    {
        MtRenderGraphPass *pass = &graph->passes[i];

        device_wait_idle(graph->dev);
        vkDestroyRenderPass(graph->dev->device, pass->render_pass.renderpass, NULL);
        pass->render_pass.renderpass = VK_NULL_HANDLE;

        for (uint32_t i = 0; i < mt_array_size(pass->framebuffers); i++)
        {
            vkDestroyFramebuffer(graph->dev->device, pass->framebuffers[i], NULL);
        }

        mt_array_free(graph->dev->alloc, pass->framebuffers);
        pass->framebuffers = NULL;
    }

    for (uint32_t i = 0; i < mt_array_size(graph->attachments); ++i)
    {
        GraphAttachment *attachment = &graph->attachments[i];
        if (attachment->image)
        {
            mt_render.destroy_image(graph->dev, attachment->image);
        }
    }

    for (uint32_t i = 0; i < mt_array_size(graph->execution_groups); ++i)
    {
        ExecutionGroup *group = &graph->execution_groups[i];
        destroy_group(graph, group);
    }
    mt_array_free(graph->dev->alloc, graph->execution_groups);
    graph->execution_groups = NULL;
}

static void graph_wait_all(MtRenderGraph *graph)
{
    for (ExecutionGroup *group = graph->execution_groups;
         group != graph->execution_groups + mt_array_size(graph->execution_groups);
         ++group)
    {
        vkWaitForFences(
            graph->dev->device,
            1,
            &group->frames[graph->current_frame].fence->fence,
            VK_TRUE,
            UINT64_MAX);
    }
}

static void graph_execute(MtRenderGraph *graph)
{
    graph->current_frame = (graph->current_frame + 1) % graph->frame_count;

    if (graph->swapchain && graph->swapchain->framebuffer_resized)
    {
        graph->swapchain->framebuffer_resized = false;
        swapchain_destroy_resizables(graph->swapchain);
        swapchain_create_resizables(graph->swapchain);
        graph_on_resize(graph);
    }

    if (graph->swapchain)
    {
        ExecutionGroup *group = mt_array_last(graph->execution_groups);
        assert(group->present_group);

        vkWaitForFences(
            graph->dev->device,
            1,
            &group->frames[graph->current_frame].fence->fence,
            VK_TRUE,
            UINT64_MAX);

        VkResult res;
        while (1)
        {
            res = vkAcquireNextImageKHR(
                graph->dev->device,
                graph->swapchain->swapchain,
                UINT64_MAX,
                graph->image_available_semaphores[graph->current_frame]->semaphore,
                VK_NULL_HANDLE,
                &graph->swapchain->current_image_index);

            if (res != VK_ERROR_OUT_OF_DATE_KHR)
            {
                break;
            }

            graph->swapchain->framebuffer_resized = true;
            return graph_execute(graph);
        }

        VK_CHECK(res);
    }

    for (ExecutionGroup *group = graph->execution_groups;
         group != graph->execution_groups + mt_array_size(graph->execution_groups);
         ++group)
    {
        vkWaitForFences(
            graph->dev->device,
            1,
            &group->frames[graph->current_frame].fence->fence,
            VK_TRUE,
            UINT64_MAX);

        MtCmdBuffer *cb = group->frames[graph->current_frame].cmd_buffer;

        mt_render.begin_cmd_buffer(cb);

        for (uint32_t i = 0; i < mt_array_size(group->pass_indices); i++)
        {
            MtRenderGraphPass *pass = &graph->passes[group->pass_indices[i]];
            if (graph->backbuffer_pass_index == pass->index && graph->swapchain)
            {
                pass->render_pass.current_framebuffer =
                    pass->framebuffers[graph->swapchain->current_image_index];
            }
            else
            {
                pass->render_pass.current_framebuffer = pass->framebuffers[0];
            }

            mt_render.cmd_begin_render_pass(cb, &pass->render_pass, NULL, NULL);

            if (pass->builder)
            {
                pass->builder(cb, graph->user_data);
            }

            mt_render.cmd_end_render_pass(cb);
        }

        mt_render.end_cmd_buffer(cb);

        mt_render.reset_fence(graph->dev, group->frames[graph->current_frame].fence);
        mt_render.submit(
            graph->dev,
            &(MtSubmitInfo){
                .cmd_buffer = cb,
                .wait_semaphore_count =
                    mt_array_size(group->frames[graph->current_frame].wait_semaphores),
                .wait_semaphores        = group->frames[graph->current_frame].wait_semaphores,
                .wait_stages            = group->frames[graph->current_frame].wait_stages,
                .signal_semaphore_count = 1,
                .signal_semaphores =
                    &group->frames[graph->current_frame].execution_finished_semaphore,
                .fence = group->frames[graph->current_frame].fence,
            });
    }

    if (graph->swapchain)
    {
        ExecutionGroup *group = mt_array_last(graph->execution_groups);
        assert(group->present_group);

        VkPresentInfoKHR present_info = {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores =
                &group->frames[graph->current_frame].execution_finished_semaphore->semaphore,
            .swapchainCount = 1,
            .pSwapchains    = &graph->swapchain->swapchain,
            .pImageIndices  = &graph->swapchain->current_image_index,
        };

        VkResult res = vkQueuePresentKHR(graph->swapchain->present_queue, &present_info);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
            graph->swapchain->framebuffer_resized = true;
        }
        else
        {
            VK_CHECK(res);
        }

        swapchain_end_frame(graph->swapchain);
    }
}

static MtImage *graph_get_attachment(MtRenderGraph *graph, const char *name)
{
    uint32_t index = (uint32_t)mt_hash_get_uint(&graph->attachment_indices, mt_hash_str(name));
    assert(index != MT_HASH_NOT_FOUND);
    return graph->attachments[index].image;
}

static MtImage *graph_consume_attachment(MtRenderGraph *graph, const char *name)
{
    uint32_t index = (uint32_t)mt_hash_get_uint(&graph->attachment_indices, mt_hash_str(name));
    assert(index != MT_HASH_NOT_FOUND);
    MtImage *image                  = graph->attachments[index].image;
    graph->attachments[index].image = NULL;
    return image;
}

static MtRenderGraphPass *
graph_add_pass(MtRenderGraph *graph, const char *name, MtPipelineStage stage)
{
    mt_array_add(graph->dev->alloc, graph->passes, 1);
    uint32_t index          = mt_array_last(graph->passes) - graph->passes;
    MtRenderGraphPass *pass = mt_array_last(graph->passes);
    memset(pass, 0, sizeof(*pass));

    pass->graph        = graph;
    pass->stage        = stage;
    pass->index        = index;
    pass->name         = name;
    pass->depth_output = UINT32_MAX;

    switch (pass->stage)
    {
        case MT_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT:
        case MT_PIPELINE_STAGE_FRAGMENT_SHADER:
        case MT_PIPELINE_STAGE_ALL_GRAPHICS: pass->queue_type = MT_QUEUE_GRAPHICS; break;
        case MT_PIPELINE_STAGE_COMPUTE: pass->queue_type = MT_QUEUE_COMPUTE; break;
        case MT_PIPELINE_STAGE_TRANSFER: pass->queue_type = MT_QUEUE_TRANSFER; break;
    }

    mt_hash_set_uint(&graph->pass_indices, mt_hash_str(name), index);
    return pass;
}

static void pass_set_builder(MtRenderGraphPass *pass, MtRenderGraphPassBuilder builder)
{
    pass->builder = builder;
}

static void pass_add_color_output(MtRenderGraphPass *pass, const char *name, MtAttachmentInfo *info)
{
    mt_array_add(pass->graph->dev->alloc, pass->graph->attachments, 1);
    uint32_t index = mt_array_last(pass->graph->attachments) - pass->graph->attachments;
    GraphAttachment *attachment = mt_array_last(pass->graph->attachments);
    memset(attachment, 0, sizeof(*attachment));

    attachment->pass_index = pass->index;
    attachment->info       = *info;

    mt_hash_set_uint(&pass->graph->attachment_indices, mt_hash_str(name), index);

    mt_array_push(pass->graph->dev->alloc, pass->color_outputs, index);
}

static void
pass_set_depth_stencil_output(MtRenderGraphPass *pass, const char *name, MtAttachmentInfo *info)
{
    mt_array_add(pass->graph->dev->alloc, pass->graph->attachments, 1);
    uint32_t index = mt_array_last(pass->graph->attachments) - pass->graph->attachments;
    GraphAttachment *attachment = mt_array_last(pass->graph->attachments);
    memset(attachment, 0, sizeof(*attachment));

    attachment->pass_index = pass->index;
    attachment->info       = *info;

    mt_hash_set_uint(&pass->graph->attachment_indices, mt_hash_str(name), index);

    pass->depth_output = index;
}

static void pass_add_attachment_input(MtRenderGraphPass *pass, const char *name)
{
    uint32_t index =
        (uint32_t)mt_hash_get_uint(&pass->graph->attachment_indices, mt_hash_str(name));
    mt_array_push(pass->graph->dev->alloc, pass->inputs, index);
}
