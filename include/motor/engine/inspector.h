#pragma once

#include <stdint.h>

typedef struct MtWindow MtWindow;
typedef struct MtEntityArchetype MtEntityArchetype;
struct nk_context;

void mt_inspect_archetype(MtWindow *window, struct nk_context *nk, MtEntityArchetype *archetype);
