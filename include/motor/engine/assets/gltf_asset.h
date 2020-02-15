#pragma once

#include "../api_types.h"
#include <motor/base/math_types.h>

typedef struct MtAssetVT MtAssetVT;
typedef struct MtCmdBuffer MtCmdBuffer;

extern MtAssetVT *mt_gltf_asset_vt;

typedef struct MtGltfAsset MtGltfAsset;

MT_ENGINE_API void mt_gltf_asset_draw(
    MtGltfAsset *asset,
    MtCmdBuffer *cb,
    Mat4 *transform,
    uint32_t model_set,
    uint32_t material_set);
