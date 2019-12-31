#pragma once

typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;
typedef struct MtAssetManager MtAssetManager;

typedef struct MtImageAsset {
    MtAssetManager *asset_manager;
    MtImage *image;
    MtSampler *sampler;
} MtImageAsset;

MtImageAsset *
mt_image_asset_create(MtAssetManager *asset_manager, const char *path);
