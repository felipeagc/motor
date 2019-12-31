static MtSampler *create_sampler(MtDevice *dev, MtSamplerCreateInfo *ci) {
    MtSampler *sampler = mt_alloc(dev->alloc, sizeof(MtSampler));
    memset(sampler, 0, sizeof(*sampler));

    if (ci->max_lod == 0.0f ||
        memcmp(&(uint32_t){0}, &ci->max_lod, sizeof(uint32_t)) == 0) {
        ci->max_lod = 1.0f;
    }

    if (ci->mag_filter == 0) {
        ci->mag_filter = MT_FILTER_LINEAR;
    }
    if (ci->min_filter == 0) {
        ci->min_filter = MT_FILTER_LINEAR;
    }
    if (ci->address_mode == 0) {
        ci->address_mode = MT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
    if (ci->border_color == 0) {
        ci->border_color = MT_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }

    VkSamplerCreateInfo sampler_create_info = {
        .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter        = filter_to_vulkan(ci->mag_filter),
        .minFilter        = filter_to_vulkan(ci->min_filter),
        .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU     = sampler_address_mode_to_vulkan(ci->address_mode),
        .addressModeV     = sampler_address_mode_to_vulkan(ci->address_mode),
        .addressModeW     = sampler_address_mode_to_vulkan(ci->address_mode),
        .mipLodBias       = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy    = 0.0f,
        .compareEnable    = VK_FALSE,
        .compareOp        = VK_COMPARE_OP_NEVER,
        .minLod           = 0.0f,
        .maxLod           = ci->max_lod,
        .borderColor      = border_color_to_vulkan(ci->border_color),
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (ci->anisotropy) {
        sampler_create_info.anisotropyEnable = VK_TRUE;
        sampler_create_info.maxAnisotropy =
            dev->physical_device_properties.limits.maxSamplerAnisotropy;
    }

    VK_CHECK(vkCreateSampler(
        dev->device, &sampler_create_info, NULL, &sampler->sampler));

    return sampler;
}

static void destroy_sampler(MtDevice *dev, MtSampler *sampler) {
    if (sampler->sampler) vkDestroySampler(dev->device, sampler->sampler, NULL);
    mt_free(dev->alloc, sampler);
}
