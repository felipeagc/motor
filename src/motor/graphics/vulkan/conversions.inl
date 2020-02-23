static VkFormat format_to_vulkan(MtFormat format)
{
    switch (format)
    {
        case MT_FORMAT_UNDEFINED: return VK_FORMAT_UNDEFINED;

        case MT_FORMAT_R8_UINT: return VK_FORMAT_R8_UINT;
        case MT_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;

        case MT_FORMAT_R8_UNORM: return VK_FORMAT_R8_UNORM;
        case MT_FORMAT_RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
        case MT_FORMAT_RGB8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
        case MT_FORMAT_RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;

        case MT_FORMAT_RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;

        case MT_FORMAT_BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        case MT_FORMAT_BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;

        case MT_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
        case MT_FORMAT_RG32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case MT_FORMAT_RGB32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
        case MT_FORMAT_RGBA32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;

        case MT_FORMAT_RG16_SFLOAT: return VK_FORMAT_R16G16_SFLOAT;
        case MT_FORMAT_RGBA16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;

        case MT_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
        case MT_FORMAT_D16_UNORM_S8_UINT: return VK_FORMAT_D16_UNORM_S8_UINT;
        case MT_FORMAT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case MT_FORMAT_D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
        case MT_FORMAT_D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;

        case MT_FORMAT_BC7_UNORM_BLOCK: return VK_FORMAT_BC7_UNORM_BLOCK;
        case MT_FORMAT_BC7_SRGB_BLOCK: return VK_FORMAT_BC7_SRGB_BLOCK;
    }

    return VK_FORMAT_UNDEFINED;
}

static VkIndexType index_type_to_vulkan(MtIndexType index_type)
{
    switch (index_type)
    {
        case MT_INDEX_TYPE_UINT16: return VK_INDEX_TYPE_UINT16;
        case MT_INDEX_TYPE_UINT32: return VK_INDEX_TYPE_UINT32;
        default: assert(0);
    }
    return 0;
}

static VkCullModeFlagBits cull_mode_to_vulkan(MtCullMode cull_mode)
{
    switch (cull_mode)
    {
        case MT_CULL_MODE_NONE: return VK_CULL_MODE_NONE;
        case MT_CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
        case MT_CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case MT_CULL_MODE_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
        default: assert(0);
    }
    return 0;
}

static VkFrontFace front_face_to_vulkan(MtFrontFace front_face)
{
    switch (front_face)
    {
        case MT_FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
        case MT_FRONT_FACE_COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        default: assert(0);
    }
    return 0;
}

static VkPolygonMode polygon_mode_to_vulkan(MtPolygonMode polygon_mode)
{
    switch (polygon_mode)
    {
        case MT_POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
        case MT_POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
        case MT_POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT;
        default: assert(0);
    }
    return 0;
}

static VkFilter filter_to_vulkan(MtFilter value)
{
    switch (value)
    {
        case MT_FILTER_LINEAR: return VK_FILTER_LINEAR;
        case MT_FILTER_NEAREST: return VK_FILTER_NEAREST;
        default: assert(0);
    }
    return 0;
}

static VkSamplerAddressMode sampler_address_mode_to_vulkan(MtSamplerAddressMode value)
{
    switch (value)
    {
        case MT_SAMPLER_ADDRESS_MODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case MT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case MT_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default: assert(0);
    }
    return 0;
}

static VkBorderColor border_color_to_vulkan(MtBorderColor value)
{
    switch (value)
    {
        case MT_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case MT_BORDER_COLOR_INT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        case MT_BORDER_COLOR_FLOAT_OPAQUE_BLACK: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case MT_BORDER_COLOR_INT_OPAQUE_BLACK: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        case MT_BORDER_COLOR_FLOAT_OPAQUE_WHITE: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        case MT_BORDER_COLOR_INT_OPAQUE_WHITE: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        default: assert(0);
    }
    return 0;
}

static VkPrimitiveTopology topology_to_vulkan(MtPrimitiveTopology value)
{
    switch (value)
    {
        case MT_PRIMITIVE_TOPOLOGY_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case MT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default: assert(0);
    }
    return 0;
}
