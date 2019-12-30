static MtFormat format_from_vulkan(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_UINT: return MT_FORMAT_R8_UINT;
    case VK_FORMAT_R32_UINT: return MT_FORMAT_R32_UINT;

    case VK_FORMAT_R8G8B8_UNORM: return MT_FORMAT_RGB8_UNORM;
    case VK_FORMAT_R8G8B8A8_UNORM: return MT_FORMAT_RGBA8_UNORM;

    case VK_FORMAT_B8G8R8A8_UNORM: return MT_FORMAT_BGRA8_UNORM;

    case VK_FORMAT_R32_SFLOAT: return MT_FORMAT_R32_SFLOAT;
    case VK_FORMAT_R32G32_SFLOAT: return MT_FORMAT_RG32_SFLOAT;
    case VK_FORMAT_R32G32B32_SFLOAT: return MT_FORMAT_RGB32_SFLOAT;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return MT_FORMAT_RGBA32_SFLOAT;

    case VK_FORMAT_R16G16B16A16_SFLOAT: return MT_FORMAT_RGBA16_SFLOAT;

    case VK_FORMAT_D16_UNORM: return MT_FORMAT_D16_UNORM;
    case VK_FORMAT_D16_UNORM_S8_UINT: return MT_FORMAT_D16_UNORM_S8_UINT;
    case VK_FORMAT_D24_UNORM_S8_UINT: return MT_FORMAT_D24_UNORM_S8_UINT;
    case VK_FORMAT_D32_SFLOAT: return MT_FORMAT_D32_SFLOAT;
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return MT_FORMAT_D32_SFLOAT_S8_UINT;

    default: return MT_FORMAT_UNDEFINED;
    }
}

static VkFormat format_to_vulkan(MtFormat format) {
    switch (format) {
    case MT_FORMAT_R8_UINT: return VK_FORMAT_R8_UINT;
    case MT_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;

    case MT_FORMAT_RGB8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
    case MT_FORMAT_RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;

    case MT_FORMAT_BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;

    case MT_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
    case MT_FORMAT_RG32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
    case MT_FORMAT_RGB32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
    case MT_FORMAT_RGBA32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;

    case MT_FORMAT_RGBA16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;

    case MT_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
    case MT_FORMAT_D16_UNORM_S8_UINT: return VK_FORMAT_D16_UNORM_S8_UINT;
    case MT_FORMAT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
    case MT_FORMAT_D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
    case MT_FORMAT_D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;

    default: return VK_FORMAT_UNDEFINED;
    }
}

static MtIndexType index_type_from_vulkan(VkIndexType index_type) {
    switch (index_type) {
    case VK_INDEX_TYPE_UINT16: return MT_INDEX_TYPE_UINT16;
    case VK_INDEX_TYPE_UINT32: return MT_INDEX_TYPE_UINT32;
    default: assert(0);
    }
}

static VkIndexType index_type_to_vulkan(MtIndexType index_type) {
    switch (index_type) {
    case MT_INDEX_TYPE_UINT16: return VK_INDEX_TYPE_UINT16;
    case MT_INDEX_TYPE_UINT32: return VK_INDEX_TYPE_UINT32;
    default: assert(0);
    }
}

static MtCullMode cull_mode_from_vulkan(VkCullModeFlagBits cull_mode) {
    switch (cull_mode) {
    case VK_CULL_MODE_NONE: return MT_CULL_MODE_NONE;
    case VK_CULL_MODE_BACK_BIT: return MT_CULL_MODE_BACK;
    case VK_CULL_MODE_FRONT_BIT: return MT_CULL_MODE_FRONT;
    case VK_CULL_MODE_FRONT_AND_BACK: return MT_CULL_MODE_FRONT_AND_BACK;
    default: assert(0);
    }
}

static VkCullModeFlagBits cull_mode_to_vulkan(MtCullMode cull_mode) {
    switch (cull_mode) {
    case MT_CULL_MODE_NONE: return VK_CULL_MODE_NONE;
    case MT_CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
    case MT_CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
    case MT_CULL_MODE_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
    default: assert(0);
    }
}

static MtFrontFace front_face_from_vulkan(VkFrontFace front_face) {
    switch (front_face) {
    case VK_FRONT_FACE_CLOCKWISE: return MT_FRONT_FACE_CLOCKWISE;
    case VK_FRONT_FACE_COUNTER_CLOCKWISE:
        return MT_FRONT_FACE_COUNTER_CLOCKWISE;
    default: assert(0);
    }
}

static VkFrontFace front_face_to_vulkan(MtFrontFace front_face) {
    switch (front_face) {
    case MT_FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
    case MT_FRONT_FACE_COUNTER_CLOCKWISE:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    default: assert(0);
    }
}

static MtFilter filter_from_vulkan(VkFilter value) {
    switch (value) {
    case VK_FILTER_LINEAR: return MT_FILTER_LINEAR;
    case VK_FILTER_NEAREST: return MT_FILTER_NEAREST;
    default: assert(0);
    }
}

static VkFilter filter_to_vulkan(MtFilter value) {
    switch (value) {
    case MT_FILTER_LINEAR: return VK_FILTER_LINEAR;
    case MT_FILTER_NEAREST: return VK_FILTER_NEAREST;
    default: assert(0);
    }
}

static MtSamplerAddressMode
sampler_address_mode_from_vulkan(VkSamplerAddressMode value) {
    switch (value) {
    case VK_SAMPLER_ADDRESS_MODE_REPEAT: return MT_SAMPLER_ADDRESS_MODE_REPEAT;
    case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
        return MT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
        return MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
        return MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
        return MT_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default: assert(0);
    }
}

static VkSamplerAddressMode
sampler_address_mode_to_vulkan(MtSamplerAddressMode value) {
    switch (value) {
    case MT_SAMPLER_ADDRESS_MODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case MT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case MT_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default: assert(0);
    }
}

static MtBorderColor border_color_from_vulkan(VkBorderColor value) {
    switch (value) {
    case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
        return MT_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
        return MT_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
        return MT_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
        return MT_BORDER_COLOR_INT_OPAQUE_BLACK;
    case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
        return MT_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
        return MT_BORDER_COLOR_INT_OPAQUE_WHITE;
    default: assert(0);
    }
}

static VkBorderColor border_color_to_vulkan(MtBorderColor value) {
    switch (value) {
    case MT_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case MT_BORDER_COLOR_INT_TRANSPARENT_BLACK:
        return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    case MT_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case MT_BORDER_COLOR_INT_OPAQUE_BLACK:
        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    case MT_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    case MT_BORDER_COLOR_INT_OPAQUE_WHITE:
        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    default: assert(0);
    }
}
