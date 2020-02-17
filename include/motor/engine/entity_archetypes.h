#pragma once

#include <motor/base/math_types.h>
#include <motor/engine/physics.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtGltfAsset MtGltfAsset;

typedef struct MtModelArchetype
{
    MtGltfAsset **model;
    MtRigidActor *actor;
    Vec3 *pos;
    Vec3 *scale;
    Quat *rot;
} MtModelArchetype;

static MtComponentSpec mt_model_archetype_components[] = {
    {"GLTF Model", sizeof(MtGltfAsset *)},
    {"Rigid actor", sizeof(MtRigidActor)},
    {"Position", sizeof(Vec3), MT_COMPONENT_TYPE_VEC3},
    {"Scale", sizeof(Vec3), MT_COMPONENT_TYPE_VEC3},
    {"Rotation", sizeof(Quat), MT_COMPONENT_TYPE_QUAT},
};

static inline void mt_model_archetype_init(void *data, MtEntity e)
{
    MtModelArchetype *comps = data;
    comps->model[e] = NULL;
    memset(&comps->actor[e], 0, sizeof(comps->actor[e]));
    comps->pos[e] = V3(0, 0, 0);
    comps->scale[e] = V3(1, 1, 1);
    comps->rot[e] = (Quat){0, 0, 0, 1};
}

typedef struct MtPointLightArchetype
{
    Vec3 *pos;
    Vec3 *color;
} MtPointLightArchetype;

static MtComponentSpec mt_light_archetype_components[] = {
    {"Position", sizeof(Vec3), MT_COMPONENT_TYPE_VEC3},
    {"Color", sizeof(Vec3), MT_COMPONENT_TYPE_VEC3},
};

static inline void mt_point_light_archetype_init(void *data, MtEntity e)
{
    MtPointLightArchetype *comps = data;
    comps->pos[e] = V3(0, 0, 0);
    comps->color[e] = V3(1, 1, 1);
}

#ifdef __cplusplus
}
#endif
