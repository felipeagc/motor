#include <motor/engine/config.h>

#include <motor/base/allocator.h>
#include <motor/base/arena.h>
#include <motor/base/bump_alloc.h>
#include <motor/base/string_builder.h>
#include <motor/base/array.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

struct MtConfig
{
    MtAllocator *alloc;
    MtAllocator bump;
    MtConfigObject root;
};

typedef enum TokenType
{
    TOKEN_NEWLINE = 1,
    TOKEN_COLON   = 2,

    TOKEN_LCURLY = 3,
    TOKEN_RCURLY = 4,

    TOKEN_LBRACK = 5,
    TOKEN_RBRACK = 6,

    TOKEN_IDENT  = 7,
    TOKEN_STRING = 8,
    TOKEN_FLOAT  = 9,
    TOKEN_INT    = 10,
    TOKEN_TRUE   = 11,
    TOKEN_FALSE  = 12,
} TokenType;

typedef struct Token
{
    TokenType type;
    union {
        const char *string;
        int64_t i64;
        double f64;
    };
} Token;

typedef struct Parser
{
    const char *input;
    uint64_t input_size;
    const char *c; // current character

    Token *tokens;
    Token *t; // current token

    MtConfig *config;
    MtStringBuilder sb;
} Parser;

static bool p_is_at_end(Parser *p)
{
    return p->t >= (p->tokens + mt_array_size(p->tokens));
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_alphanum(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static bool is_numeric(char c)
{
    return ((c >= '0' && c <= '9') || c == '.' || c == '-');
}

static bool s_is_at_end(Parser *p)
{
    return (uint64_t)(p->c - p->input) >= p->input_size || *p->c == '\0';
}

static bool scan_multiline_string(Parser *p, Token *token)
{
    token->type = TOKEN_STRING;

    if (s_is_at_end(p) || strncmp(p->c, "[[", 2) != 0)
    {
        return false;
    }
    p->c += 2;

    mt_str_builder_reset(&p->sb);
    while (!s_is_at_end(p) && strncmp(p->c, "]]", 2) != 0)
    {
        mt_str_builder_append_char(&p->sb, *p->c);
        p->c++;
    }
    token->string = mt_str_builder_build(&p->sb, &p->config->bump);

    if (s_is_at_end(p) || strncmp(p->c, "]]", 2) != 0)
    {
        return false;
    }
    p->c += 2;

    return true;
}

static bool scan_string(Parser *p, Token *token)
{
    token->type = TOKEN_STRING;

    if (s_is_at_end(p) || *p->c != '"')
        return false;
    p->c++;

    mt_str_builder_reset(&p->sb);
    while (!s_is_at_end(p) && *p->c != '"')
    {
        mt_str_builder_append_char(&p->sb, *p->c);
        p->c++;
    }
    token->string = mt_str_builder_build(&p->sb, &p->config->bump);

    if (s_is_at_end(p) || *p->c != '"')
        return false;
    p->c++;

    return true;
}

static bool scan_identifier(Parser *p, Token *token)
{
    char ident[128];
    char *k = ident;
    while (!s_is_at_end(p) && is_alphanum(*p->c))
    {
        *(k++) = *(p->c++);
    }
    *k = '\0';

    if (strcmp(ident, "true") == 0)
    {
        token->type = TOKEN_TRUE;
        return true;
    }

    if (strcmp(ident, "false") == 0)
    {
        token->type = TOKEN_FALSE;
        return true;
    }

    token->type   = TOKEN_IDENT;
    token->string = mt_strdup(&p->config->bump, ident);

    return true;
}

static bool scan_number(Parser *p, Token *token)
{
    bool found_dot = false;

    char str[48];
    char *s = str;
    while (!s_is_at_end(p) && ((*p->c >= '0' && *p->c <= '9') || *p->c == '-' || *p->c == '.'))
    {
        if (*p->c == '.')
            found_dot = true;
        *(s++) = *(p->c++);
    }
    *s = '\0';

    if (found_dot)
    {
        token->type = TOKEN_FLOAT;
        token->f64  = strtod(str, NULL);
    }
    else
    {
        token->type = TOKEN_INT;
        token->i64  = strtol(str, NULL, 10);
    }

    return true;
}

static bool scan_token(Parser *p)
{
    if (s_is_at_end(p))
        return true;

    Token token = {0};

    switch (*p->c)
    {
        case ' ':
        case '\r':
        case '\t':
        {
            p->c++;
            return true;
        }
        case '\n':
        {
            p->c++;
            token.type = TOKEN_NEWLINE;
            break;
        }
        case ':':
        {
            p->c++;
            token.type = TOKEN_COLON;
            break;
        }
        case '{':
        {
            p->c++;
            token.type = TOKEN_LCURLY;
            break;
        }
        case '}':
        {
            p->c++;
            token.type = TOKEN_RCURLY;
            break;
        }
        case '[':
        {
            if (*(p->c + 1) == '[')
            {
                if (!scan_multiline_string(p, &token))
                {
                    return false;
                }
                break;
            }
            p->c++;
            token.type = TOKEN_LBRACK;
        }
        break;
        case ']':
        {
            p->c++;
            token.type = TOKEN_RBRACK;
        }
        break;
        case '"':
        {
            if (!scan_string(p, &token))
            {
                return false;
            }
            break;
        }
        case '/':
        {
            if (*(p->c + 1) == '/')
            {
                // Comment
                while (!s_is_at_end(p) && *p->c != '\n')
                {
                    p->c++;
                }
                return true;
            }

            return false;
        }
        default:
        {
            if (is_alpha(*p->c))
            {
                if (!scan_identifier(p, &token))
                {
                    return false;
                }
                break;
            }
            if (is_numeric(*p->c))
            {
                if (!scan_number(p, &token))
                {
                    return false;
                }
                break;
            }

            return false;
        }
    }

    mt_array_push(p->config->alloc, p->tokens, token);

    return true;
}

static bool parser_scan(Parser *p)
{
    while (!s_is_at_end(p))
    {
        if (!scan_token(p))
            return false;
    }
    return true;
}

static bool parse_object_entries(Parser *p, MtConfigObject *obj);

static bool parse_entry(Parser *p, MtConfigEntry *entry)
{
    if (p_is_at_end(p) || p->t->type != TOKEN_IDENT)
    {
        return false;
    }
    entry->key = p->t->string;
    p->t++;

    if (p_is_at_end(p) || p->t->type != TOKEN_COLON)
    {
        return false;
    }
    p->t++;

    if (p_is_at_end(p))
    {
        return false;
    }

    switch (p->t->type)
    {
        case TOKEN_STRING:
        {
            entry->value.type   = MT_CONFIG_VALUE_STRING;
            entry->value.string = p->t->string;
            p->t++;
            break;
        }
        case TOKEN_TRUE:
        {
            entry->value.type    = MT_CONFIG_VALUE_BOOL;
            entry->value.boolean = true;
            p->t++;
            break;
        }
        case TOKEN_FALSE:
        {
            entry->value.type    = MT_CONFIG_VALUE_BOOL;
            entry->value.boolean = false;
            p->t++;
            break;
        }
        case TOKEN_INT:
        {
            entry->value.type = MT_CONFIG_VALUE_INT;
            entry->value.i64  = p->t->i64;
            p->t++;
            break;
        }
        case TOKEN_FLOAT:
        {
            entry->value.type = MT_CONFIG_VALUE_FLOAT;
            entry->value.f64  = p->t->f64;
            p->t++;
            break;
        }
        case TOKEN_LCURLY:
        {
            entry->value.type = MT_CONFIG_VALUE_OBJECT;
            p->t++;

            while (!p_is_at_end(p) && p->t->type == TOKEN_NEWLINE)
            {
                p->t++;
            }

            if (p_is_at_end(p) || !parse_object_entries(p, &entry->value.object))
            {
                return false;
            }

            while (!p_is_at_end(p) && p->t->type == TOKEN_NEWLINE)
            {
                p->t++;
            }

            if (p_is_at_end(p) || p->t->type != TOKEN_RCURLY)
            {
                return false;
            }
            p->t++;
            break;
        }
        default: return false;
    }

    while (!p_is_at_end(p) && p->t->type == TOKEN_NEWLINE)
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
    while (!p_is_at_end(p) && p->t->type != TOKEN_RCURLY && (res = parse_entry(p, &entry)))
    {
        uint64_t key_hash = mt_hash_str(entry.key);
        if (!mt_hash_get_ptr(&obj->map, key_hash))
        {
            MtConfigEntry *entry_ptr = mt_array_push(p->config->alloc, obj->entries, entry);
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

    mt_bump_alloc_init(&config->bump, 1 << 12, config->alloc);

    Parser parser     = {0};
    parser.input      = input;
    parser.input_size = input_size;
    parser.config     = config;

    mt_str_builder_init(&parser.sb, config->alloc);

    parser.c = input;
    if (!parser_scan(&parser))
    {
        mt_str_builder_destroy(&parser.sb);
        mt_array_free(alloc, parser.tokens);
        mt_free(alloc, config);
        return NULL;
    }

    parser.t = parser.tokens;
    if (!parse_object_entries(&parser, &config->root))
    {
        mt_str_builder_destroy(&parser.sb);
        mt_array_free(alloc, parser.tokens);
        mt_free(alloc, config);
        return NULL;
    }

    mt_array_free(alloc, parser.tokens);

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
        mt_bump_alloc_destroy(&config->bump);
        mt_free(config->alloc, config);
    }
}
