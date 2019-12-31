#pragma once

typedef struct MtAsset MtAsset;

typedef struct MtAssetVT {
    void (*destroy)(MtAsset *inst);
} MtAssetVT;

typedef struct MtIAsset {
    MtAsset *inst;
    MtAssetVT *vt;
} MtIAsset;
