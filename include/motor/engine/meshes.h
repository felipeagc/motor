#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtBuffer MtBuffer;
typedef struct MtCmdBuffer MtCmdBuffer;

typedef struct MtMesh
{
    MtEngine *engine;
    MtBuffer *vertex_buffer;
    uint32_t vertex_count;
} MtMesh;

MT_ENGINE_API void mt_mesh_init_sphere(MtMesh *mesh, MtEngine *engine);

MT_ENGINE_API void mt_mesh_draw(MtMesh *mesh, MtCmdBuffer *cb);

MT_ENGINE_API void mt_mesh_destroy(MtMesh *mesh);

#ifdef __cplusplus
}
#endif
