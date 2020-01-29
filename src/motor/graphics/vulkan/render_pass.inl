static MtRenderPass *create_render_pass(MtDevice *dev, MtRenderPassCreateInfo *ci)
{
    MtRenderPass *rp = mt_alloc(dev->alloc, sizeof(MtRenderPass));
    memset(rp, 0, sizeof(*rp));

    if (ci->color_attachments != NULL && ci->color_attachment_count == 0)
    {
        ci->color_attachment_count = 1;
    }

    rp->color_attachment_count = ci->color_attachment_count;

    if (ci->depth_attachment)
    {
        rp->has_depth_attachment = true;
    }

    for (uint32_t i = 0; i < ci->color_attachment_count; ++i)
    {
        if (rp->extent.width == 0 && rp->extent.height == 0)
        {
            rp->extent.width  = ci->color_attachments[i]->width;
            rp->extent.height = ci->color_attachments[i]->height;
        }

        if (rp->extent.width != ci->color_attachments[i]->width ||
            rp->extent.height != ci->color_attachments[i]->height)
        {
            mt_free(dev->alloc, rp);
            return NULL;
        }
    }

    if (ci->depth_attachment)
    {
        if (rp->extent.width == 0 && rp->extent.height == 0)
        {
            rp->extent.width  = ci->depth_attachment->width;
            rp->extent.height = ci->depth_attachment->height;
        }

        if (rp->extent.width != ci->depth_attachment->width ||
            rp->extent.height != ci->depth_attachment->height)
        {
            mt_free(dev->alloc, rp);
            return NULL;
        }
    }

    VkAttachmentDescription *attachments = NULL;
    uint32_t *color_attachment_indices   = NULL;
    uint32_t depth_attachment_index      = UINT32_MAX;

    for (uint32_t i = 0; i < ci->color_attachment_count; ++i)
    {
        mt_array_push(dev->alloc, color_attachment_indices, mt_array_size(attachments));

        VkAttachmentDescription color_attachment = {
            .format         = ci->color_attachments[i]->format,
            .samples        = ci->color_attachments[i]->sample_count,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        mt_array_push(dev->alloc, attachments, color_attachment);
    }

    if (ci->depth_attachment)
    {
        depth_attachment_index = mt_array_size(attachments);

        VkAttachmentDescription depth_attachment = {
            .format         = ci->depth_attachment->format,
            .samples        = ci->depth_attachment->sample_count,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        };

        mt_array_push(dev->alloc, attachments, depth_attachment);
    }

    VkAttachmentReference *color_attachment_refs = NULL;

    for (uint32_t i = 0; i < ci->color_attachment_count; ++i)
    {
        VkAttachmentReference ref = {
            .attachment = color_attachment_indices[i],
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        mt_array_push(dev->alloc, color_attachment_refs, ref);
    }

    VkAttachmentReference depth_attachment_ref = {
        .attachment = depth_attachment_index,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

    if (ci->color_attachment_count > 0)
    {
        subpass.colorAttachmentCount = mt_array_size(color_attachment_refs);
        subpass.pColorAttachments    = color_attachment_refs;
    }

    if (ci->depth_attachment)
    {
        subpass.pDepthStencilAttachment = &depth_attachment_ref;
    }

    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderpass_create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = mt_array_size(attachments),
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };

    rp->hash = vulkan_hash_render_pass(&renderpass_create_info);

    VK_CHECK(vkCreateRenderPass(dev->device, &renderpass_create_info, NULL, &rp->renderpass));

    VkImageView *image_views = NULL;

    for (uint32_t i = 0; i < ci->color_attachment_count; ++i)
    {
        mt_array_push(dev->alloc, image_views, ci->color_attachments[i]->image_view);
    }

    if (ci->depth_attachment)
    {
        mt_array_push(dev->alloc, image_views, ci->depth_attachment->image_view);
    }

    VkFramebufferCreateInfo framebuffer_create_info = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = rp->renderpass,
        .attachmentCount = mt_array_size(image_views),
        .pAttachments    = image_views,
        .width           = rp->extent.width,
        .height          = rp->extent.height,
        .layers          = 1,
    };

    VK_CHECK(
        vkCreateFramebuffer(dev->device, &framebuffer_create_info, NULL, &rp->current_framebuffer));

    mt_array_free(dev->alloc, color_attachment_indices);
    mt_array_free(dev->alloc, color_attachment_refs);
    mt_array_free(dev->alloc, attachments);
    mt_array_free(dev->alloc, image_views);

    return rp;
}

static void destroy_render_pass(MtDevice *dev, MtRenderPass *render_pass)
{
    device_wait_idle(dev);

    vkDestroyFramebuffer(dev->device, render_pass->current_framebuffer, NULL);
    vkDestroyRenderPass(dev->device, render_pass->renderpass, NULL);
}
