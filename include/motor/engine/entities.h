#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;
typedef struct MtEntityManager MtEntityManager;
typedef struct MtBufferWriter MtBufferWriter;

#ifndef MT_COMP_BIT
#define MT_COMP_BIT(components, component) (1 << (offsetof(components, component) / sizeof(void *)))
#endif

#define MT_ENTITY_INVALID UINT32_MAX

typedef uint32_t MtEntity;
typedef uint32_t MtComponentMask;
typedef uint32_t MtComponentType;

typedef void (*MtEntityInitializer)(MtEntityManager *, MtEntity entity);
typedef void (*MtEntitySerializer)(MtEntityManager *, MtBufferWriter *);

typedef struct MtComponentSpec
{
    const char *name;
    uint32_t size;
    MtComponentType type;
} MtComponentSpec;

typedef struct MtEntityDescriptor
{
    MtEntityInitializer entity_init;
    MtEntitySerializer entity_serialize;
    const MtComponentSpec *component_specs;
    uint32_t component_spec_count;
} MtEntityDescriptor;

typedef struct MtEntityManager
{
    MtAllocator *alloc;

    MtComponentSpec *component_specs;
    uint32_t component_spec_count;
    MtEntityInitializer entity_init;
    MtEntitySerializer entity_serialize;

    MtEntity selected_entity;

    uint32_t entity_count;
    uint32_t entity_cap;
    MtComponentMask *masks;
    void **components;
} MtEntityManager;

MT_ENGINE_API void mt_entity_manager_init(
    MtEntityManager *em, MtAllocator *alloc, const MtEntityDescriptor *descriptor);

MT_ENGINE_API void mt_entity_manager_destroy(MtEntityManager *em);

MT_ENGINE_API void mt_entity_manager_serialize(MtEntityManager *em, MtBufferWriter *bw);

MT_ENGINE_API MtEntity
mt_entity_manager_add_entity(MtEntityManager *em, MtComponentMask component_mask);

#ifdef __cplusplus
}
#endif
