#pragma once

typedef struct MtPipeline MtPipeline;
typedef struct MtIAsset MtIAsset;
typedef struct MtAssetManager MtAssetManager;

typedef struct MtPipelineAsset {
    MtAssetManager *asset_manager;
    MtPipeline *pipeline;
} MtPipelineAsset;

MtIAsset *
mt_pipeline_asset_create(MtAssetManager *asset_manager, const char *path);
