static void buffer_pool_init(
    MtDevice *dev, BufferPool *pool, size_t block_size, size_t alignment, MtBufferUsage usage)
{
    memset(pool, 0, sizeof(*pool));
    pool->dev = dev;
    pool->block_size = block_size;
    pool->alignment = alignment;
    pool->usage = usage;
}

static void buffer_block_destroy(BufferPool *pool, BufferBlock *block)
{
    unmap_buffer(pool->dev, block->buffer);
    destroy_buffer(pool->dev, block->buffer);
}

static void buffer_pool_destroy(BufferPool *pool)
{
    for (uint32_t i = 0; i < mt_array_size(pool->blocks); i++)
    {
        buffer_block_destroy(pool, &pool->blocks[i]);
    }
    mt_array_free(pool->dev->alloc, pool->blocks);
}

static void buffer_pool_recycle(BufferPool *pool, BufferBlock *block)
{
    if (block->mapping != NULL)
    {
        assert(block->buffer->buffer);
        mt_array_push(pool->dev->alloc, pool->blocks, *block);
    }
    memset(block, 0, sizeof(*block));
}

static BufferBlock buffer_pool_allocate_block(BufferPool *pool, size_t size)
{
    BufferBlock block = {0};
    block.size = size;
    block.offset = 0;
    block.alignment = pool->alignment;

    block.buffer = create_buffer(
        pool->dev,
        &(MtBufferCreateInfo){
            .size = size,
            .usage = pool->usage,
            .memory = MT_BUFFER_MEMORY_HOST,
        });

    block.mapping = map_buffer(pool->dev, block.buffer);

    assert(block.buffer->buffer);

    return block;
}

static BufferBlock buffer_pool_request_block(BufferPool *pool, size_t minimum_size)
{
    if (pool->usage == MT_BUFFER_USAGE_UNIFORM)
    {
        assert(minimum_size <= pool->block_size);
    }
    if (minimum_size > pool->block_size || mt_array_size(pool->blocks) == 0)
    {
        // Allocate new block
        return buffer_pool_allocate_block(pool, MT_MAX(minimum_size, pool->block_size));
    }
    else
    {
        // Use existing block

        // Pop last block from blocks
        BufferBlock block = *mt_array_pop(pool->blocks);
        block.offset = 0;
        assert(block.buffer->buffer);

        return block;
    }
    assert(0);
}

static BufferBlockAllocation buffer_block_allocate(BufferBlock *block, size_t allocate_size)
{
    BufferBlockAllocation allocation = {0};

    assert(block->size >= allocate_size);

    size_t aligned_offset = (block->offset + block->alignment - 1) & ~(block->alignment - 1);
    assert(aligned_offset % block->alignment == 0);
    if (aligned_offset + allocate_size <= block->size)
    {
        uint8_t *ret = block->mapping + aligned_offset;
        block->offset = aligned_offset + allocate_size;

        allocation.mapping = ret;
        allocation.offset = aligned_offset;
        allocation.size = allocate_size;
    }

    return allocation;
}

static void buffer_block_reset(BufferBlock *block)
{
    if (block->mapping)
    {
        block->offset = 0;
        assert(block->buffer->buffer);
    }
}

static BufferBlock *
ensure_buffer_block(BufferPool *pool, BufferBlock **blocks, size_t allocate_size)
{
    for (BufferBlock *block = *blocks; block != (*blocks) + mt_array_size(*blocks); ++block)
    {
        size_t aligned_offset = (block->offset + block->alignment - 1) & ~(block->alignment - 1);
        if (block->mapping != NULL && block->size >= aligned_offset + allocate_size)
        {
            return block;
        }
    }

    BufferBlock new_block = buffer_pool_request_block(pool, allocate_size);
    mt_array_push(pool->dev->alloc, *blocks, new_block);

    assert(mt_array_last(*blocks)->alignment == new_block.alignment);

    return mt_array_last(*blocks);
}
