#include <motor/engine/config.h>

#include <motor/base/lexer.h>
#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

struct MtConfig
{
    MtAllocator *alloc;
    MtConfigObject root;
};

typedef struct Parser
{
    MtToken *tokens;
    size_t token_count;
    MtToken *t; // current token

    MtConfig *config;
} Parser;

static inline bool is_at_end(Parser *p)
{
    return p->t >= (p->tokens + p->token_count);
}

static bool parse_object_entries(Parser *p, MtConfigObject *obj);

static bool parse_entry(Parser *p, MtConfigEntry *entry)
{
    if (is_at_end(p) || p->t->type != MT_TOKEN_IDENT)
    {
        return false;
    }
    entry->key        = p->t->string;
    entry->key_length = p->t->length;
    p->t++;

    if (is_at_end(p) || p->t->type != MT_TOKEN_EQUAL)
    {
        return false;
    }
    p->t++;

    if (is_at_end(p))
    {
        return false;
    }

    switch (p->t->type)
    {
        case MT_TOKEN_STRING:
        {
            entry->value.type   = MT_CONFIG_VALUE_STRING;
            entry->value.string = p->t->string;
            entry->value.length = p->t->length;
            p->t++;
            break;
        }
        case MT_TOKEN_IDENT:
        {
            if (strncmp(p->t->string, "true", p->t->length) == 0)
            {
                entry->value.type    = MT_CONFIG_VALUE_BOOL;
                entry->value.boolean = true;
                p->t++;
                break;
            }
            else if (strncmp(p->t->string, "false", p->t->length) == 0)
            {
                entry->value.type    = MT_CONFIG_VALUE_BOOL;
                entry->value.boolean = false;
                p->t++;
                break;
            }
            return false;
        }
        case MT_TOKEN_INT:
        {
            entry->value.type = MT_CONFIG_VALUE_INT;
            entry->value.i64  = p->t->i64;
            p->t++;
            break;
        }
        case MT_TOKEN_FLOAT:
        {
            entry->value.type = MT_CONFIG_VALUE_FLOAT;
            entry->value.f64  = p->t->f64;
            p->t++;
            break;
        }
        case MT_TOKEN_LCURLY:
        {
            entry->value.type = MT_CONFIG_VALUE_OBJECT;
            p->t++;

            while (!is_at_end(p) && p->t->type == MT_TOKEN_NEWLINE)
            {
                p->t++;
            }

            if (is_at_end(p) || !parse_object_entries(p, &entry->value.object))
            {
                return false;
            }

            while (!is_at_end(p) && p->t->type == MT_TOKEN_NEWLINE)
            {
                p->t++;
            }

            if (is_at_end(p) || p->t->type != MT_TOKEN_RCURLY)
            {
                return false;
            }
            p->t++;
            break;
        }
        default:
        {
            return false;
        }
    }

    while (!is_at_end(p) && p->t->type == MT_TOKEN_NEWLINE)
    {
        p->t++;
    }

    return true;
}

static bool parse_object_entries(Parser *p, MtConfigObject *obj)
{
    memset(obj, 0, sizeof(*obj));
    mt_hash_init(&obj->map, 11, p->config->alloc);

    bool res = true;
    MtConfigEntry entry;
    while (!is_at_end(p) && p->t->type != MT_TOKEN_RCURLY && (res = parse_entry(p, &entry)))
    {
        uint64_t key_hash = mt_hash_strn(entry.key, entry.key_length);
        if (!mt_hash_get_ptr(&obj->map, key_hash))
        {
            mt_array_push(p->config->alloc, obj->entries, entry);
            MtConfigEntry *entry_ptr = mt_array_last(obj->entries);
            mt_hash_set_ptr(&obj->map, key_hash, entry_ptr);
        }
        else
        {
            return false;
        }
    }

    return res;
}

static void destroy_object(MtConfig *config, MtConfigObject *obj)
{
    for (uint32_t i = 0; i < mt_array_size(obj->entries); i++)
    {
        if (obj->entries[i].value.type == MT_CONFIG_VALUE_OBJECT)
        {
            destroy_object(config, &obj->entries[i].value.object);
        }
    }
    mt_hash_destroy(&obj->map);
    mt_array_free(config->alloc, obj->entries);
}

MtConfig *mt_config_parse(MtAllocator *alloc, const char *input, uint64_t input_size)
{
    MtConfig *config = mt_alloc(alloc, sizeof(MtConfig));
    memset(config, 0, sizeof(*config));

    config->alloc = alloc;

    Parser parser = {0};
    parser.config = config;
    parser.tokens = mt_lexer_scan(input, input_size, alloc, &parser.token_count);

    parser.t = parser.tokens;
    if (!parse_object_entries(&parser, &config->root))
    {
        mt_free(alloc, parser.tokens);
        mt_free(alloc, config);
        return NULL;
    }

    mt_free(alloc, parser.tokens);

    return config;
}

MtConfigObject *mt_config_get_root(MtConfig *config)
{
    return &config->root;
}

void mt_config_destroy(MtConfig *config)
{
    if (config)
    {
        destroy_object(config, &config->root);
        mt_free(config->alloc, config);
    }
}
