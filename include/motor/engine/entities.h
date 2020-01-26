#pragma once

#include <stdint.h>

typedef struct MtAllocator MtAllocator;

enum
{
    MT_ENTITIES_PER_BLOCK = 32
};

typedef struct MtEntityBlock
{
    uint32_t entity_count;
    void *data;
} MtEntityBlock;

typedef void (*MtEntityInitializer)(void *data, uint32_t index);

typedef struct MtEntityArchetype
{
    uint64_t block_size;
    MtEntityInitializer entity_init;
    /*array*/ MtEntityBlock *blocks;
} MtEntityArchetype;

typedef struct MtEntityManager
{
    MtAllocator *alloc;
    /*array*/ MtEntityArchetype *archetypes;
} MtEntityManager;

void mt_entity_manager_init(MtEntityManager *em, MtAllocator *alloc);

void mt_entity_manager_destroy(MtEntityManager *em);

// Returns the index of the archetype
uint32_t mt_entity_manager_register_archetype(
    MtEntityManager *em, MtEntityInitializer entity_init, uint64_t block_size);

void *
mt_entity_manager_add_entity(MtEntityManager *em, uint32_t archetype_index, uint32_t *entity_index);
