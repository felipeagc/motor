#pragma once

typedef struct MtPipeline MtPipeline;
typedef struct MtAssetManager MtAssetManager;

typedef struct MtPipelineAsset {
    MtAssetManager *asset_manager;
    MtPipeline *pipeline;
} MtPipelineAsset;

MtPipelineAsset *
mt_pipeline_asset_create(MtAssetManager *asset_manager, const char *path);
