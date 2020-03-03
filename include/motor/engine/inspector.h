#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtEntityManager MtEntityManager;

MT_ENGINE_API void mt_inspect_entities(MtEngine *engine, MtEntityManager *em);

#ifdef __cplusplus
}
#endif
