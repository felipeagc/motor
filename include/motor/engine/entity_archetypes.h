#pragma once

#include <motor/base/math.h>

typedef struct MtGltfAsset MtGltfAsset;

typedef struct MtModelArchetype
{
    MtGltfAsset *model[MT_ENTITIES_PER_BLOCK];
    Vec3 pos[MT_ENTITIES_PER_BLOCK];
    Vec3 scale[MT_ENTITIES_PER_BLOCK];
    Quat rot[MT_ENTITIES_PER_BLOCK];
} MtModelArchetype;

static inline void mt_model_archetype_init(void *data, uint32_t i)
{
    MtModelArchetype *block = data;
    block->model[i]         = NULL;
    block->pos[i]           = V3(0, 0, 0);
    block->scale[i]         = V3(1, 1, 1);
    block->rot[i]           = (Quat){0, 0, 0, 1};
}

typedef struct MtPointLightArchetype
{
    Vec3 pos[MT_ENTITIES_PER_BLOCK];
    Vec3 color[MT_ENTITIES_PER_BLOCK];
} MtPointLightArchetype;

static inline void mt_point_light_archetype_init(void *data, uint32_t i)
{
    MtPointLightArchetype *block = data;
    block->pos[i]                = V3(0, 0, 0);
    block->color[i]              = V3(1, 1, 1);
}
