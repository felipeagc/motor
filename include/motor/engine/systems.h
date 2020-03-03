#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEntityManager MtEntityManager;
typedef struct MtCmdBuffer MtCmdBuffer;
typedef struct MtPipeline MtPipeline;
typedef struct MtEnvironment MtEnvironment;
typedef struct MtPerspectiveCamera MtPerspectiveCamera;
typedef struct MtScene MtScene;

MT_ENGINE_API void mt_light_system(MtEntityManager *em, MtScene *scene, float delta);

MT_ENGINE_API void mt_model_system(MtEntityManager *em, MtScene *scene, MtCmdBuffer *cb);

MT_ENGINE_API void mt_selected_entity_system(MtEntityManager *em, MtScene *scene, MtCmdBuffer *cb);

MT_ENGINE_API void mt_picking_system(MtCmdBuffer *cb, void *user_data);

MT_ENGINE_API void mt_pre_physics_sync_system(MtEntityManager *em);

MT_ENGINE_API void mt_post_physics_sync_system(MtEntityManager *em);

#ifdef __cplusplus
}
#endif
