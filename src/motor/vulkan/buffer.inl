static MtBuffer *create_buffer(MtDevice *dev, MtBufferCreateInfo *ci) {
    MtBuffer *buffer = mt_alloc(dev->arena, sizeof(MtBuffer));
    assert(ci->size != 0);

    buffer->size   = ci->size;
    buffer->usage  = ci->usage;
    buffer->memory = ci->memory;

    VkBufferUsageFlags buffer_usage       = 0;
    VmaMemoryUsage memory_usage           = 0;
    VkMemoryPropertyFlags memory_property = 0;

    switch (buffer->usage) {
    case MT_BUFFER_USAGE_VERTEX: {
        buffer_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    } break;
    case MT_BUFFER_USAGE_INDEX: {
        buffer_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    } break;
    case MT_BUFFER_USAGE_UNIFORM: {
        buffer_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    } break;
    case MT_BUFFER_USAGE_STORAGE: {
        buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    } break;
    case MT_BUFFER_USAGE_TRANSFER: {
        buffer_usage =
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    } break;
    }

    switch (buffer->memory) {
    case MT_BUFFER_MEMORY_HOST: {
        memory_usage    = VMA_MEMORY_USAGE_CPU_TO_GPU;
        memory_property = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    } break;
    case MT_BUFFER_MEMORY_DEVICE: {
        memory_usage    = VMA_MEMORY_USAGE_GPU_ONLY;
        memory_property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        buffer_usage |=
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    } break;
    }

    VkBufferCreateInfo create_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = buffer->size,
        .usage       = buffer_usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo alloc_create_info = {
        .usage         = memory_usage,
        .requiredFlags = memory_property,
    };

    VK_CHECK(vmaCreateBuffer(
        dev->gpu_allocator,
        &create_info,
        &alloc_create_info,
        &buffer->buffer,
        &buffer->allocation,
        NULL));

    return buffer;
}

static void destroy_buffer(MtDevice *dev, MtBuffer *buffer) {
    if (!buffer) return;

    VK_CHECK(vkDeviceWaitIdle(dev->device));
    if (buffer->buffer != VK_NULL_HANDLE &&
        buffer->allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(
            dev->gpu_allocator, buffer->buffer, buffer->allocation);
    }
    mt_free(dev->arena, buffer);
}

static void *map_buffer(MtDevice *dev, MtBuffer *buffer) {
    void *dest;
    VK_CHECK(vmaMapMemory(dev->gpu_allocator, buffer->allocation, &dest));
    return dest;
}

static void unmap_buffer(MtDevice *dev, MtBuffer *buffer) {
    vmaUnmapMemory(dev->gpu_allocator, buffer->allocation);
}
