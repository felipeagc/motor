#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtEntityArchetype MtEntityArchetype;

MT_ENGINE_API void mt_inspect_archetype(MtEngine *engine, MtEntityArchetype *archetype);

#ifdef __cplusplus
}
#endif
