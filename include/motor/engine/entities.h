#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;
typedef struct MtEntityManager MtEntityManager;

#ifndef MT_COMP_BIT
#define MT_COMP_BIT(archetype, component) (1 << (offsetof(archetype, component) / sizeof(void *)))
#endif

#define MT_ENTITY_INVALID UINT32_MAX

typedef uint32_t MtEntity;
typedef uint32_t MtComponentMask;
typedef void (*MtEntityInitializer)(void *data, MtEntity entity);

typedef enum MtComponentType {
    MT_COMPONENT_TYPE_UNKNOWN = 0,
    MT_COMPONENT_TYPE_TRANSFORM,
    MT_COMPONENT_TYPE_VEC3,
    MT_COMPONENT_TYPE_QUAT,
} MtComponentType;

typedef struct MtComponentSpec
{
    const char *name;
    size_t size;
    MtComponentType type;
} MtComponentSpec;

typedef struct MtArchetypeSpec
{
    MtComponentSpec *components;
    uint32_t component_count;
} MtArchetypeSpec;

typedef struct MtEntityArchetype
{
    uint32_t entity_count;
    uint32_t entity_cap;
    MtEntityInitializer entity_init;

    MtEntity selected_entity;

    MtComponentMask *masks;
    void **components;
    MtArchetypeSpec spec;
} MtEntityArchetype;

typedef struct MtEntityManager
{
    MtAllocator *alloc;
    MtEntityArchetype archetypes[128];
    uint32_t archetype_count;
} MtEntityManager;

MT_ENGINE_API void mt_entity_manager_init(MtEntityManager *em, MtAllocator *alloc);

MT_ENGINE_API void mt_entity_manager_destroy(MtEntityManager *em);

MT_ENGINE_API MtEntityArchetype *mt_entity_manager_register_archetype(
    MtEntityManager *em,
    MtComponentSpec *components,
    uint32_t component_count,
    MtEntityInitializer initializer);

MT_ENGINE_API MtEntity mt_entity_manager_add_entity(
    MtEntityManager *em, MtEntityArchetype *archetype, MtComponentMask component_mask);

#ifdef __cplusplus
}
#endif
