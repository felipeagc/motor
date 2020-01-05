#pragma once

typedef struct MtAssetVT MtAssetVT;
typedef struct MtCmdBuffer MtCmdBuffer;

extern MtAssetVT *mt_gltf_asset_vt;

typedef struct MtGltfAsset MtGltfAsset;

void mt_gltf_asset_draw(MtGltfAsset *asset, MtCmdBuffer *cb);
