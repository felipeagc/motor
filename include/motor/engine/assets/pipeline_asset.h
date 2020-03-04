#pragma once

#include <motor/engine/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtPipeline MtPipeline;

typedef struct MtAssetManager MtAssetManager;
typedef struct MtIAsset MtIAsset;
typedef struct MtAssetVT MtAssetVT;

extern MtAssetVT *mt_pipeline_asset_vt;

typedef struct MtPipelineAsset
{
    MtAsset asset;
    MtAssetManager *asset_manager;
    MtPipeline *pipeline;
} MtPipelineAsset;

#ifdef __cplusplus
}
#endif
