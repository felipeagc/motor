#pragma once

#include "api_types.h"
#include <motor/engine/camera.h>
#include <motor/engine/environment.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtPhysicsScene MtPhysicsScene;
typedef struct MtAssetManager MtAssetManager;
typedef struct MtEntityManager MtEntityManager;
typedef struct MtRenderGraph MtRenderGraph;
typedef struct MtScene MtScene;
typedef struct MtEvent MtEvent;

typedef struct MtSceneVT
{
    void (*init)(MtScene *inst);
    void (*update)(MtScene *inst, float delta);
    void (*draw_ui)(MtScene *inst);
    void (*on_event)(MtScene *inst, const MtEvent *event);
    void (*destroy)(MtScene *inst);
} MtSceneVT;

typedef struct MtIScene
{
    MtScene *inst;
    const MtSceneVT *vt;
} MtIScene;

struct MtScene
{
    MtEngine *engine;

    MtPerspectiveCamera cam;
    MtEnvironment env;

    MtRenderGraph *graph;

    MtPhysicsScene *physics_scene;

    MtAssetManager *asset_manager;
    MtEntityManager *entity_manager;
};

MT_ENGINE_API void mt_scene_init(MtScene *scene, MtEngine *engine);

MT_ENGINE_API void mt_scene_destroy(MtScene *scene);

#ifdef __cplusplus
}
#endif
