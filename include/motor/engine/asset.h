#pragma once

#include "api_types.h"

#ifdef __cpluspus
extern "C" {
#endif

typedef struct MtAsset MtAsset;
typedef struct MtAssetManager MtAssetManager;

typedef struct MtAssetVT
{
    const char *name;

    const char **extensions;
    uint32_t extension_count;

    size_t size;

    bool (*init)(MtAssetManager *, MtAsset *, const char *path);
    void (*destroy)(MtAsset *inst);
} MtAssetVT;

typedef struct MtIAsset
{
    MtAsset *inst;
    MtAssetVT *vt;
} MtIAsset;

#ifdef __cpluspus
}
#endif
