#pragma once

#include "api_types.h"
#include <motor/base/math_types.h>
#include <motor/engine/entities.h>
#include <motor/engine/physics.h>
#include <motor/engine/transform.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtGltfAsset MtGltfAsset;

typedef struct MtModelArchetype
{
    MtTransform *transform;
    MtGltfAsset **model;
    MtRigidActor *actor;
} MtModelArchetype;

static MtComponentSpec mt_model_archetype_components[] = {
    {"Transform", sizeof(MtTransform)},
    {"GLTF Model", sizeof(MtGltfAsset *)},
    {"Rigid actor", sizeof(MtRigidActor)},
};

static inline void mt_model_archetype_init(void *data, MtEntity e)
{
    MtModelArchetype *comps = data;
    comps->transform[e].pos = V3(0.f, 0.f, 0.f);
    comps->transform[e].scale = V3(1.f, 1.f, 1.f);
    comps->transform[e].rot = (Quat){0, 0, 0, 1};
    comps->model[e] = NULL;
    memset(&comps->actor[e], 0, sizeof(comps->actor[e]));
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
