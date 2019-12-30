static void buffer_pool_init(
    MtDevice *dev,
    BufferPool *pool,
    size_t block_size,
    size_t alignment,
    size_t spill_size,
    MtBufferUsage usage) {
    memset(pool, 0, sizeof(*pool));
    pool->dev        = dev;
    pool->block_size = block_size;
    pool->alignment  = alignment;
    pool->spill_size = spill_size;
    pool->usage      = usage;
}

static void buffer_block_destroy(BufferPool *pool, BufferBlock *block) {
    unmap_buffer(pool->dev, block->buffer);
    destroy_buffer(pool->dev, block->buffer);
}

static void buffer_pool_destroy(BufferPool *pool) {
    for (uint32_t i = 0; i < mt_array_size(pool->blocks); i++) {
        buffer_block_destroy(pool, &pool->blocks[i]);
    }
}

static void buffer_pool_recycle(BufferPool *pool, BufferBlock *block) {
    if (block->mapping != NULL) {
        mt_array_push(pool->dev->arena, pool->blocks, *block);
    }
    memset(block, 0, sizeof(*block));
}

static BufferBlock buffer_pool_allocate_block(BufferPool *pool, size_t size) {
    BufferBlock block = {0};
    block.size        = size;
    block.offset      = 0;
    block.alignment   = pool->alignment;
    block.spill_size  = pool->spill_size;

    block.buffer = create_buffer(
        pool->dev,
        &(MtBufferCreateInfo){
            .size   = size,
            .usage  = pool->usage,
            .memory = MT_BUFFER_MEMORY_HOST,
        });

    block.mapping = map_buffer(pool->dev, block.buffer);

    return block;
}

static BufferBlock
buffer_pool_request_block(BufferPool *pool, size_t minimum_size) {
    if (minimum_size > pool->block_size || mt_array_size(pool->blocks) == 0) {
        // Allocate new block
        return buffer_pool_allocate_block(
            pool, MT_MAX(minimum_size, pool->block_size));
    } else {
        // Use existing block

        // Pop last block from blocks
        BufferBlock block = *mt_array_pop(pool->blocks);
        block.offset      = 0;

        return block;
    }
}

static BufferBlockAllocation
buffer_block_allocate(BufferBlock *block, size_t allocate_size) {
    BufferBlockAllocation allocation = {0};

    size_t aligned_offset =
        (block->offset + block->alignment - 1) & ~(block->alignment - 1);
    if (aligned_offset + allocate_size <= block->size) {
        uint8_t *ret  = block->mapping + aligned_offset;
        block->offset = aligned_offset + allocate_size;

        size_t padded_size = MT_MAX(allocate_size, block->spill_size);
        padded_size        = MT_MIN(padded_size, block->size - aligned_offset);

        allocation.mapping     = ret;
        allocation.offset      = aligned_offset;
        allocation.padded_size = padded_size;
    }

    return allocation;
}

static void buffer_block_reset(BufferBlock *block) { block->offset = 0; }

MT_INLINE void ensure_buffer_block(
    BufferPool *pool, BufferBlock *block, size_t allocate_size) {
    size_t aligned_offset =
        (block->offset + block->alignment - 1) & ~(block->alignment - 1);
    if (block->mapping == NULL ||
        block->size < aligned_offset + allocate_size) {
        buffer_pool_recycle(pool, block);

        *block =
            buffer_pool_request_block(pool, aligned_offset + allocate_size);
    }
}
