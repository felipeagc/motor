#pragma once

#include <motor/engine/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;

typedef struct MtAssetManager MtAssetManager;
typedef struct MtAssetVT MtAssetVT;

extern MtAssetVT *mt_image_asset_vt;

typedef struct MtImageAsset
{
    MtAsset asset;
    MtAssetManager *asset_manager;
    MtImage *image;
} MtImageAsset;

#ifdef __cplusplus
}
#endif
