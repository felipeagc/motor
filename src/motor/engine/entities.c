#include <motor/engine/entities.h>

#include <motor/base/array.h>
#include <motor/base/allocator.h>
#include <string.h>

void mt_entity_manager_init(MtEntityManager *em, MtAllocator *alloc)
{
    memset(em, 0, sizeof(*em));
    em->alloc = alloc;
}

void mt_entity_manager_destroy(MtEntityManager *em)
{
    for (uint32_t i = 0; i < mt_array_size(em->archetypes); ++i)
    {
        MtEntityArchetype *archetype = &em->archetypes[i];
        for (uint32_t j = 0; j < mt_array_size(archetype->blocks); ++j)
        {
            mt_free(em->alloc, archetype->blocks[j].data);
        }
        mt_array_free(em->alloc, archetype->blocks);
    }
    mt_array_free(em->alloc, em->archetypes);
}

uint32_t mt_entity_manager_register_archetype(
    MtEntityManager *em, MtEntityInitializer entity_init, uint64_t block_size)
{
    MtEntityArchetype archetype = {0};
    archetype.entity_init       = entity_init;
    archetype.block_size        = block_size;
    mt_array_push(em->alloc, em->archetypes, archetype);
    return mt_array_size(em->archetypes) - 1;
}

void *
mt_entity_manager_add_entity(MtEntityManager *em, uint32_t archetype_index, uint32_t *entity_index)
{
    if (mt_array_size(em->archetypes) <= archetype_index)
    {
        // Archetype is not registered
        return NULL;
    }

    MtEntityArchetype *archetype = &em->archetypes[archetype_index];

    while (1)
    {
        for (uint32_t i = 0; i < mt_array_size(archetype->blocks); ++i)
        {
            MtEntityBlock *block = &archetype->blocks[i];
            if (block->entity_count < MT_ENTITIES_PER_BLOCK)
            {
                *entity_index = block->entity_count;
                block->entity_count++;

                archetype->entity_init(block->data, *entity_index);

                return block->data;
            }
        }

        MtEntityBlock new_block = {0};
        new_block.data          = mt_alloc(em->alloc, archetype->block_size);
        mt_array_push(em->alloc, archetype->blocks, new_block);
    }

    return NULL;
}
