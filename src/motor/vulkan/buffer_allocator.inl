static void buffer_page_init(
    BufferAllocatorPage *page,
    BufferAllocator *allocator,
    size_t page_size,
    MtBufferUsage usage) {
    memset(page, 0, sizeof(*page));

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(allocator->dev->physical_device, &properties);

    page->part_size = properties.limits.minUniformBufferOffsetAlignment;

    size_t part_count = page_size / page->part_size;

    mt_dynamic_bitset_init(&page->in_use, part_count, allocator->dev->arena);

    page->buffer = create_buffer(
        allocator->dev,
        &(MtBufferCreateInfo){
            .size   = page_size,
            .usage  = usage,
            .memory = MT_BUFFER_MEMORY_HOST,
        });
    assert(page->buffer);

    page->mapping = map_buffer(allocator->dev, page->buffer);
}

static void
buffer_page_destroy(BufferAllocatorPage *page, BufferAllocator *allocator) {
    if (page->next) buffer_page_destroy(page->next, allocator);
    unmap_buffer(allocator->dev, page->buffer);

    // TODO: mt_Free pages

    destroy_buffer(allocator->dev, page->buffer);
    mt_dynamic_bitset_destroy(&page->in_use, allocator->dev->arena);
}

static void buffer_page_begin_frame(BufferAllocatorPage *page) {
    mt_dynamic_bitset_clear(&page->in_use);
    page->last_index = 0;
}

static void *buffer_page_allocate(
    BufferAllocatorPage *page,
    size_t size,
    MtBuffer **out_buffer,
    size_t *out_offset) {
    if (size <= page->buffer->size) {
        size_t required_parts = (size + page->part_size - 1) / page->part_size;
        for (uint32_t i = page->last_index;
             i <= page->in_use.nbits - required_parts;
             i++) {
            if (page->in_use.nbits <= i + required_parts) {
                break;
            }

            bool good = true;
            for (uint32_t j = i; j < i + required_parts; j++) {
                if (mt_bitset_get(&page->in_use, j)) {
                    good = false;
                    break;
                }
            }

            if (good) {
                *out_buffer = page->buffer;
                *out_offset = i * page->part_size;

                for (uint32_t j = i; j < i + required_parts; j++) {
                    mt_bitset_enable(&page->in_use, j);
                }

                page->last_index = i + (uint32_t)required_parts;

                return page->mapping + (i * page->part_size);
            }
        }
    }

    return NULL;
}

static void buffer_allocator_init(
    BufferAllocator *allocator,
    MtDevice *dev,
    size_t page_size,
    MtBufferUsage usage) {
    allocator->usage     = usage;
    allocator->dev       = dev;
    allocator->page_size = page_size;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        buffer_page_init(
            &allocator->base_pages[i], allocator, page_size, usage);
    }
}

static void buffer_allocator_destroy(BufferAllocator *allocator) {
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        buffer_page_destroy(&allocator->base_pages[i], allocator);
    }
}

static void buffer_allocator_begin_frame(BufferAllocator *allocator) {
    allocator->current_frame =
        (allocator->current_frame + 1) % FRAMES_IN_FLIGHT;

    BufferAllocatorPage *page =
        &allocator->base_pages[allocator->current_frame];
    while (page) {
        buffer_page_begin_frame(page);
        page = page->next;
    }
}

static void *buffer_allocator_allocate(
    BufferAllocator *allocator,
    size_t size,
    MtBuffer **buffer,
    size_t *offset) {
    BufferAllocatorPage *page =
        &allocator->base_pages[allocator->current_frame];
    while (page) {
        void *memory = buffer_page_allocate(page, size, buffer, offset);
        if (memory) return memory;
        if (!page->next) break;
        page = page->next;
    }

    assert(!page->next);

    size_t new_page_size = allocator->page_size;
    if (size > new_page_size) new_page_size = size * 2;

    page->next = mt_alloc(allocator->dev->arena, sizeof(BufferAllocatorPage));
    buffer_page_init(page->next, allocator, new_page_size, allocator->usage);
    page = page->next;

    return buffer_page_allocate(page, size, buffer, offset);
}
