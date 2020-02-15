#pragma once

#include "api_types.h"

typedef struct MtWindow MtWindow;
typedef struct MtEntityArchetype MtEntityArchetype;
struct nk_context;

MT_ENGINE_API void
mt_inspect_archetype(MtWindow *window, struct nk_context *nk, MtEntityArchetype *archetype);
