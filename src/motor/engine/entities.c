#include <motor/engine/entities.h>

#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <string.h>
#include <assert.h>

void mt_entity_manager_init(
    MtEntityManager *em, MtAllocator *alloc, MtScene *scene, const MtEntityDescriptor *descriptor)
{
    memset(em, 0, sizeof(*em));
    em->alloc = alloc;
    em->scene = scene;

    em->component_spec_count = descriptor->component_spec_count;
    em->component_specs =
        mt_alloc(em->alloc, sizeof(MtComponentSpec) * descriptor->component_spec_count);
    memcpy(
        em->component_specs,
        descriptor->component_specs,
        sizeof(MtComponentSpec) * descriptor->component_spec_count);

    em->components = mt_alloc(em->alloc, sizeof(void *) * descriptor->component_spec_count);
    memset(em->components, 0, sizeof(void *) * descriptor->component_spec_count);

    em->entity_serialize = descriptor->entity_serialize;
    em->entity_deserialize = descriptor->entity_deserialize;

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

    for (uint32_t c = 0; c < em->component_spec_count; ++c)
    {
        MtComponentSpec *comp_spec = &em->component_specs[c];
        uint8_t *comp_data = em->components[c] + (em->component_specs[c].size * entity_index);
        memset(comp_data, 0, em->component_specs[c].size);

        if ((em->masks[entity_index] & (1 << c)) == (1 << c))
        {
            if (comp_spec->init)
            {
                comp_spec->init(em, comp_data);
            }
        }
    }

    return entity_index;
}

void mt_entity_manager_remove_entity(MtEntityManager *em, MtEntity entity)
{
    for (uint32_t c = 0; c < em->component_spec_count; ++c)
    {
        MtComponentSpec *comp_spec = &em->component_specs[c];
        uint8_t *comp_data = em->components[c];

        if (comp_spec->uninit)
        {
            comp_spec->uninit(em, comp_data + (em->component_specs[c].size * entity), true);
        }

        memcpy(
            comp_data + (em->component_specs[c].size * entity),
            comp_data + (em->component_specs[c].size * (em->entity_count - 1)),
            em->component_specs[c].size);
        em->masks[entity] = em->masks[em->entity_count - 1];
    }
    em->entity_count--;
    em->selected_entity = MT_ENTITY_INVALID;
}

void mt_entity_manager_serialize(MtEntityManager *em, MtBufferWriter *bw)
{
    em->entity_serialize(em, bw);
}

void mt_entity_manager_deserialize(MtEntityManager *em, MtBufferReader *br)
{
    em->entity_deserialize(em, br);
}
