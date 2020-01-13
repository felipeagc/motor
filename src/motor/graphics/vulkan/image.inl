static MtImage *create_image(MtDevice *dev, MtImageCreateInfo *ci)
{
    MtImage *image = mt_alloc(dev->alloc, sizeof(MtImage));
    memset(image, 0, sizeof(*image));

    if (ci->depth == 0)
    {
        ci->depth = 1;
    }
    if (ci->sample_count == 0)
    {
        ci->sample_count = 1;
    }
    if (ci->mip_count == 0)
    {
        ci->mip_count = 1;
    }
    if (ci->layer_count == 0)
    {
        ci->layer_count = 1;
    }
    if (ci->format == 0)
    {
        ci->format = MT_FORMAT_RGBA8_UNORM;
    }
    if (ci->usage == 0)
    {
        ci->usage = MT_IMAGE_USAGE_TRANSFER_DST_BIT | MT_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (ci->aspect == 0)
    {
        ci->aspect = MT_IMAGE_ASPECT_COLOR_BIT;
    }

    image->width       = ci->width;
    image->height      = ci->height;
    image->depth       = ci->depth;
    image->mip_count   = ci->mip_count;
    image->layer_count = ci->layer_count;
    image->format      = format_to_vulkan(ci->format);

    assert(image->format != VK_FORMAT_UNDEFINED);

    switch (ci->sample_count)
    {
        case 1: image->sample_count = VK_SAMPLE_COUNT_1_BIT; break;
        case 2: image->sample_count = VK_SAMPLE_COUNT_2_BIT; break;
        case 4: image->sample_count = VK_SAMPLE_COUNT_4_BIT; break;
        case 8: image->sample_count = VK_SAMPLE_COUNT_8_BIT; break;
        case 16: image->sample_count = VK_SAMPLE_COUNT_16_BIT; break;
        default: assert(0);
    }

    if (ci->aspect & MT_IMAGE_ASPECT_COLOR_BIT)
        image->aspect |= VK_IMAGE_ASPECT_COLOR_BIT;
    if (ci->aspect & MT_IMAGE_ASPECT_DEPTH_BIT)
        image->aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (ci->aspect & MT_IMAGE_ASPECT_STENCIL_BIT)
        image->aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;

    // Create image
    {
        VkImageCreateInfo image_create_info = {
            .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format    = image->format,
            .extent =
                {
                    .width  = image->width,
                    .height = image->height,
                    .depth  = image->depth,
                },
            .mipLevels     = image->mip_count,
            .arrayLayers   = image->layer_count,
            .samples       = image->sample_count,
            .tiling        = VK_IMAGE_TILING_OPTIMAL,
            .usage         = 0,
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        // Usages
        if (ci->usage & MT_IMAGE_USAGE_SAMPLED_BIT)
            image_create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (ci->usage & MT_IMAGE_USAGE_STORAGE_BIT)
            image_create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (ci->usage & MT_IMAGE_USAGE_TRANSFER_SRC_BIT)
            image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (ci->usage & MT_IMAGE_USAGE_TRANSFER_DST_BIT)
            image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (ci->usage & MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            image_create_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (ci->usage & MT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            image_create_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        if (image->layer_count == 6)
        {
            image_create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        VmaAllocationCreateInfo image_alloc_create_info = {0};
        image_alloc_create_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK(vmaCreateImage(
            dev->gpu_allocator,
            &image_create_info,
            &image_alloc_create_info,
            &image->image,
            &image->allocation,
            NULL));
    }

    // Create image view
    {
        VkImageViewCreateInfo image_view_create_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = image->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = image->format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {
                    .aspectMask     = image->aspect,
                    .baseMipLevel   = 0,
                    .levelCount     = image->mip_count,
                    .baseArrayLayer = 0,
                    .layerCount     = image->layer_count,
                },
        };

        if (image->layer_count == 6)
        {
            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }

        VK_CHECK(vkCreateImageView(dev->device, &image_view_create_info, NULL, &image->image_view));
    }

    return image;
}

static void destroy_image(MtDevice *dev, MtImage *image)
{
    device_wait_idle(dev);

    if (image->image_view)
        vkDestroyImageView(dev->device, image->image_view, NULL);
    if (image->image)
        vmaDestroyImage(dev->gpu_allocator, image->image, image->allocation);
    mt_free(dev->alloc, image);
}
