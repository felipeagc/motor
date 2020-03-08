#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;
typedef struct MtEntityManager MtEntityManager;
typedef struct MtScene MtScene;
typedef struct MtBufferWriter MtBufferWriter;
typedef struct MtBufferReader MtBufferReader;

#ifndef MT_COMP_BIT
#define MT_COMP_BIT(components, component) (1 << (offsetof(components, component) / sizeof(void *)))
#endif

#define MT_ENTITY_INVALID UINT32_MAX

typedef uint32_t MtEntity;
typedef uint32_t MtComponentMask;
typedef uint32_t MtComponentType;

typedef void (*MtComponentInitializer)(MtEntityManager *, void *comp);
typedef void (*MtComponentUninitializer)(MtEntityManager *, void *comp);
typedef void (*MtComponentUnregisterer)(MtEntityManager *, void *comp);
typedef void (*MtEntitySerializer)(MtEntityManager *, MtBufferWriter *);
typedef void (*MtEntityDeserializer)(MtEntityManager *, MtBufferReader *);

typedef struct MtComponentSpec
{
    const char *name;
    uint32_t size;
    MtComponentType type;
    MtComponentInitializer init;
    MtComponentUninitializer uninit;
    MtComponentUnregisterer unregister; // Example usage: removing physics object from scene without
                                        // deleting the physics object itself
} MtComponentSpec;

typedef struct MtEntityDescriptor
{
    MtEntitySerializer entity_serialize;
    MtEntityDeserializer entity_deserialize;
    const MtComponentSpec *component_specs;
    uint32_t component_spec_count;
} MtEntityDescriptor;

typedef struct MtEntityManager
{
    MtAllocator *alloc;

    MtScene *scene;

    MtComponentSpec *component_specs;
    uint32_t component_spec_count;
    MtEntitySerializer entity_serialize;
    MtEntityDeserializer entity_deserialize;

    MtEntity selected_entity;

    uint32_t entity_count;
    uint32_t entity_cap;
    MtComponentMask *masks;
    void **components;
} MtEntityManager;

MT_ENGINE_API void mt_entity_manager_init(
    MtEntityManager *em, MtAllocator *alloc, MtScene *scene, const MtEntityDescriptor *descriptor);

MT_ENGINE_API void mt_entity_manager_destroy(MtEntityManager *em);

MT_ENGINE_API void mt_entity_manager_serialize(MtEntityManager *em, MtBufferWriter *bw);

MT_ENGINE_API void mt_entity_manager_deserialize(MtEntityManager *em, MtBufferReader *br);

MT_ENGINE_API MtEntity
mt_entity_manager_add_entity(MtEntityManager *em, MtComponentMask component_mask);

MT_ENGINE_API void mt_entity_manager_remove_entity(MtEntityManager *em, MtEntity entity);

#ifdef __cplusplus
}
#endif
