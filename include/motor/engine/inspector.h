#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtWindow MtWindow;
typedef struct MtEntityArchetype MtEntityArchetype;

MT_ENGINE_API void mt_inspect_archetype(MtWindow *window, MtEntityArchetype *archetype);

#ifdef __cplusplus
}
#endif
