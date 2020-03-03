#include <motor/engine/entities.h>

#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <string.h>
#include <assert.h>

void mt_entity_manager_init(
    MtEntityManager *em, MtAllocator *alloc, const MtEntityDescriptor *descriptor)
{
    memset(em, 0, sizeof(*em));
    em->alloc = alloc;

    em->component_spec_count = descriptor->component_spec_count;
    em->component_specs =
        mt_alloc(em->alloc, sizeof(MtComponentSpec) * descriptor->component_spec_count);
    memcpy(
        em->component_specs,
        descriptor->component_specs,
        sizeof(MtComponentSpec) * descriptor->component_spec_count);

    em->components = mt_alloc(em->alloc, sizeof(void *) * descriptor->component_spec_count);
    memset(em->components, 0, sizeof(void *) * descriptor->component_spec_count);

    em->entity_init = descriptor->entity_init;

    em->selected_entity = MT_ENTITY_INVALID;
}

void mt_entity_manager_destroy(MtEntityManager *em)
{
    for (uint32_t c = 0; c < em->component_spec_count; ++c)
    {
        mt_free(em->alloc, em->components[c]);
    }
    mt_free(em->alloc, em->component_specs);
    mt_free(em->alloc, em->components);
}

MtEntity mt_entity_manager_add_entity(MtEntityManager *em, MtComponentMask component_mask)
{
    if (em->entity_count >= em->entity_cap)
    {
        em->entity_cap *= 2;
        if (em->entity_cap == 0)
        {
            em->entity_cap = 32; // Initial cap
        }

        em->masks = mt_realloc(em->alloc, em->masks, sizeof(*em->masks) * em->entity_cap);

        for (uint32_t c = 0; c < em->component_spec_count; ++c)
        {
            assert(em->component_specs[c].size > 0);

            em->components[c] = mt_realloc(
                em->alloc, em->components[c], em->component_specs[c].size * em->entity_cap);
        }
    }

    MtEntity entity_index = em->entity_count++;
    em->masks[entity_index] = component_mask;

    if (em->entity_init)
    {
        em->entity_init(em, entity_index);
    }
    else
    {
        for (uint32_t c = 0; c < em->component_spec_count; ++c)
        {
            memset(&em->components[c], 0, em->component_specs[c].size);
        }
    }

    return entity_index;
}
