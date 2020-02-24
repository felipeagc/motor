#include <motor/engine/entities.h>

#include <motor/base/log.h>
#include <motor/base/array.h>
#include <motor/base/allocator.h>
#include <string.h>
#include <assert.h>

void mt_entity_manager_init(MtEntityManager *em, MtAllocator *alloc)
{
    memset(em, 0, sizeof(*em));
    em->alloc = alloc;
}

void mt_entity_manager_destroy(MtEntityManager *em)
{
    for (MtEntityArchetype *archetype = em->archetypes;
         archetype != em->archetypes + mt_array_size(em->archetypes);
         archetype++)
    {
        for (uint32_t i = 0; i < archetype->spec.component_count; ++i)
        {
            mt_free(em->alloc, archetype->components[i]);
        }
        mt_free(em->alloc, archetype->spec.components);
        mt_free(em->alloc, archetype->components);
    }
}

MtEntityArchetype *mt_entity_manager_register_archetype(
    MtEntityManager *em,
    MtComponentSpec *components,
    uint32_t component_count,
    MtEntityInitializer initializer)
{
    uint32_t arch_index = em->archetype_count++;
    MtEntityArchetype *archetype = &em->archetypes[arch_index];

    archetype->spec.component_count = component_count;
    archetype->spec.components = mt_alloc(em->alloc, sizeof(MtComponentSpec) * component_count);
    memcpy(archetype->spec.components, components, sizeof(MtComponentSpec) * component_count);

    archetype->components = mt_alloc(em->alloc, sizeof(void *) * component_count);
    memset(archetype->components, 0, sizeof(void *) * component_count);

    archetype->entity_init = initializer;

    archetype->selected_entity = MT_ENTITY_INVALID;

    return archetype;
}

MtEntity mt_entity_manager_add_entity(
    MtEntityManager *em, MtEntityArchetype *archetype, MtComponentMask component_mask)
{
    if ((archetype - em->archetypes) >= em->archetype_count)
    {
        // Archetype is not registered
        mt_log_error("Tried to add entity to invalid archetype");
        return MT_ENTITY_INVALID;
    }

    if (archetype->entity_count >= archetype->entity_cap)
    {
        archetype->entity_cap *= 2;
        if (archetype->entity_cap == 0)
        {
            archetype->entity_cap = 32; // Initial cap
        }

        archetype->masks = mt_realloc(
            em->alloc, archetype->masks, sizeof(*archetype->masks) * archetype->entity_cap);

        for (uint32_t i = 0; i < archetype->spec.component_count; ++i)
        {
            assert(archetype->spec.components[i].size > 0);

            archetype->components[i] = mt_realloc(
                em->alloc,
                archetype->components[i],
                archetype->spec.components[i].size * archetype->entity_cap);
        }
    }

    MtEntity entity_index = archetype->entity_count++;
    archetype->masks[entity_index] = component_mask;

    if (archetype->entity_init)
    {
        archetype->entity_init(archetype->components, entity_index);
    }
    else
    {
        for (uint32_t i = 0; i < archetype->spec.component_count; ++i)
        {
            memset(&archetype->components[i], 0, archetype->spec.components[i].size);
        }
    }

    return entity_index;
}
