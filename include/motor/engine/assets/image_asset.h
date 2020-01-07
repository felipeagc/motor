#pragma once

typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;

typedef struct MtAssetManager MtAssetManager;
typedef struct MtAssetVT MtAssetVT;

extern MtAssetVT *mt_image_asset_vt;

typedef struct MtImageAsset
{
    MtAssetManager *asset_manager;
    MtImage *image;
} MtImageAsset;
