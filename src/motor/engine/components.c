#include <motor/engine/components.h>

#include <motor/base/buffer_writer.h>
#include <motor/base/array.h>

static MtComponentSpec default_component_specs[] = {
    {"Transform", sizeof(MtTransform), MT_COMPONENT_TYPE_TRANSFORM},
    {"GLTF Model", sizeof(MtGltfAsset *), MT_COMPONENT_TYPE_GLTF_MODEL},
    {"Rigid actor", sizeof(MtRigidActor *), MT_COMPONENT_TYPE_RIGID_ACTOR},
    {"Point light", sizeof(MtPointLightComponent), MT_COMPONENT_TYPE_POINT_LIGHT},
};

static void default_entity_init(MtEntityManager *em, MtEntity e)
{
    MtDefaultComponents *comps = (MtDefaultComponents *)em->components;
    comps->transform[e].pos = V3(0.f, 0.f, 0.f);
    comps->transform[e].scale = V3(1.f, 1.f, 1.f);
    comps->transform[e].rot = (Quat){0, 0, 0, 1};
    comps->model[e] = NULL;
    comps->actor[e] = NULL;
    comps->point_light[e].color = V3(1, 1, 1);
    comps->point_light[e].radius = 0.0f;
}

static void default_entity_serialize(MtEntityManager *em, MtBufferWriter *bw)
{
    mt_buffer_writer_append(bw, &em->entity_count, sizeof(em->entity_count));
    for (MtEntity e = 0; e < em->entity_count; ++e)
    {
    }
}

MT_ENGINE_API MtEntityDescriptor mt_default_entity_descriptor = {
    .entity_init = default_entity_init,
    .entity_serialize = default_entity_serialize,
    .component_specs = default_component_specs,
    .component_spec_count = MT_LENGTH(default_component_specs),
};
