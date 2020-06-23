#pragma once

#include "api_types.h"
#include "gizmos.h"
#include "scene.h"
#include "meshes.h"
#include <motor/base/thread_pool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtDevice MtDevice;
typedef struct MtWindow MtWindow;
typedef struct MtSwapchain MtSwapchain;
typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;
typedef struct MtBuffer MtBuffer;
typedef struct MtFileWatcher MtFileWatcher;
typedef struct MtPhysics MtPhysics;
typedef struct MtPicker MtPicker;
typedef struct MtAssetManager MtAssetManager;
typedef struct MtImguiContext MtImguiContext;
typedef struct shaderc_compiler *shaderc_compiler_t;
typedef struct MtPipelineAsset MtPipelineAsset;
typedef struct MtGltfAsset MtGltfAsset;

typedef struct MtEngine
{
    MtDevice *device;
    MtWindow *window;
    MtSwapchain *swapchain;

    MtPhysics *physics;
    MtPicker *picker;
    MtGizmo gizmo;

    MtAllocator *alloc;
    MtThreadPool thread_pool;
    MtAssetManager *asset_manager;

    MtImguiContext *imgui_ctx;
    MtFileWatcher *watcher;

    shaderc_compiler_t compiler;

    MtImage *white_image;
    MtImage *black_image;
    MtImage *default_cubemap;
    MtSampler *default_sampler;

    MtMesh sphere_mesh;

    MtGltfAsset *default_cube;

    MtPipelineAsset *pbr_pipeline;
    MtPipelineAsset *wireframe_pipeline;
    MtPipelineAsset *gizmo_pipeline;
    MtPipelineAsset *skybox_pipeline;
    MtPipelineAsset *brdf_pipeline;
    MtPipelineAsset *irradiance_pipeline;
    MtPipelineAsset *prefilter_env_pipeline;
    MtPipelineAsset *imgui_pipeline;
    MtPipelineAsset *picking_pipeline;
    MtPipelineAsset *picking_transfer_pipeline;

    bool playing;

    MtIScene current_scene;
} MtEngine;

MT_ENGINE_API void mt_engine_init(MtEngine *engine);

MT_ENGINE_API void mt_engine_destroy(MtEngine *engine);

MT_ENGINE_API void mt_engine_set_scene(MtEngine *engine, const MtIScene *scene);

MT_ENGINE_API void mt_engine_update(MtEngine *engine);

#ifdef __cplusplus
}
#endif
