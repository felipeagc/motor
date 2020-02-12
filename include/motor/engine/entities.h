#pragma once

#include <stdint.h>
#include <stddef.h>

#ifndef MT_COMP_INDEX
#define MT_COMP_INDEX(archetype, component) (offsetof(archetype, component) / sizeof(void *))
#endif

typedef struct MtAllocator MtAllocator;
typedef struct MtEntityManager MtEntityManager;

typedef void (*MtEntityInitializer)(void *data, uint64_t index);

typedef struct MtComponentSpec
{
    const char *name;
    size_t size;
} MtComponentSpec;

typedef struct MtArchetypeSpec
{
    MtComponentSpec *components;
    uint64_t component_count;
} MtArchetypeSpec;

typedef struct MtEntityArchetype
{
    uint64_t entity_count;
    uint64_t entity_cap;
    MtEntityInitializer entity_init;

    void **components;
    MtArchetypeSpec spec;
} MtEntityArchetype;

typedef struct MtEntityManager
{
    MtAllocator *alloc;
    MtEntityArchetype archetypes[128];
    uint64_t archetype_count;
} MtEntityManager;

void mt_entity_manager_init(MtEntityManager *em, MtAllocator *alloc);

void mt_entity_manager_destroy(MtEntityManager *em);

MtEntityArchetype *mt_entity_manager_register_archetype(
    MtEntityManager *em,
    MtComponentSpec *components,
    uint64_t component_count,
    MtEntityInitializer initializer);

uint64_t mt_entity_manager_add_entity(MtEntityManager *em, MtEntityArchetype *archetype);
