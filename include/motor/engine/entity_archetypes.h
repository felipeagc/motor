#pragma once

#include <motor/base/math.h>

#ifdef __cpluspus
extern "C" {
#endif

typedef struct MtGltfAsset MtGltfAsset;

typedef struct MtModelArchetype
{
    MtGltfAsset **model;
    Vec3 *pos;
    Vec3 *scale;
    Quat *rot;
} MtModelArchetype;

static MtComponentSpec mt_model_archetype_components[] = {
    {"GLTF Model", sizeof(MtGltfAsset *)},
    {"Position", sizeof(Vec3), MT_COMPONENT_TYPE_VEC3},
    {"Scale", sizeof(Vec3), MT_COMPONENT_TYPE_VEC3},
    {"Rotation", sizeof(Quat), MT_COMPONENT_TYPE_QUAT},
};

static inline void mt_model_archetype_init(void *data, MtEntity e)
{
    MtModelArchetype *comps = data;
    comps->model[e] = NULL;
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

#ifdef __cpluspus
}
#endif
