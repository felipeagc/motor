#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;
typedef struct MtEntityManager MtEntityManager;

#ifndef MT_COMP_BIT
#define MT_COMP_BIT(components, component) (1 << (offsetof(components, component) / sizeof(void *)))
#endif

#define MT_ENTITY_INVALID UINT32_MAX

typedef uint32_t MtEntity;
typedef uint32_t MtComponentMask;
typedef void (*MtEntityInitializer)(MtEntityManager *, MtEntity entity);

typedef enum MtComponentType {
    MT_COMPONENT_TYPE_UNKNOWN = 0,
    MT_COMPONENT_TYPE_TRANSFORM,
    MT_COMPONENT_TYPE_VEC3,
    MT_COMPONENT_TYPE_QUAT,
    MT_COMPONENT_TYPE_RIGID_ACTOR,
    MT_COMPONENT_TYPE_GLTF_MODEL,
} MtComponentType;

typedef struct MtComponentSpec
{
    const char *name;
    uint32_t size;
    MtComponentType type;
} MtComponentSpec;

typedef struct MtEntityDescriptor
{
    MtEntityInitializer entity_init;
    const MtComponentSpec *component_specs;
    uint32_t component_spec_count;
} MtEntityDescriptor;

typedef struct MtEntityManager
{
    MtAllocator *alloc;

    MtComponentSpec *component_specs;
    uint32_t component_spec_count;
    MtEntityInitializer entity_init;

    MtEntity selected_entity;

    uint32_t entity_count;
    uint32_t entity_cap;
    MtComponentMask *masks;
    void **components;
} MtEntityManager;

MT_ENGINE_API void mt_entity_manager_init(
    MtEntityManager *em, MtAllocator *alloc, const MtEntityDescriptor *descriptor);

MT_ENGINE_API void mt_entity_manager_destroy(MtEntityManager *em);

MT_ENGINE_API MtEntity
mt_entity_manager_add_entity(MtEntityManager *em, MtComponentMask component_mask);

#ifdef __cplusplus
}
#endif
