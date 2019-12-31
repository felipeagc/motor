#include "../../include/motor/config.h"

#include "../../include/motor/arena.h"
#include "../../include/motor/bump_alloc.h"
#include "../../include/motor/array.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct MtConfig {
    MtAllocator bump;
    MtAllocator alloc;
    MtStringBuilder sb;
    MtConfigObject root;
};

typedef struct Parser {
    char *input;
    uint64_t input_size;
    char *c;
    MtConfig *config;
} Parser;

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_alphanum(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static bool is_at_end(Parser *p) {
    return (uint64_t)(p->c - p->input) >= p->input_size || *p->c == '\0';
}

static bool skip_whitespace(Parser *p) {
    while (!is_at_end(p) &&
           (*p->c == ' ' || *p->c == '\t' || *p->c == '\n' || *p->c == '\r')) {
        p->c++;
    }

    return true;
}

static bool skip_whitespace1(Parser *p) {
    uint32_t spaces = 0;
    while (!is_at_end(p) && ((*p->c) == ' ' || (*p->c) == '\t' ||
                             (*p->c) == '\n' || (*p->c) == '\r')) {
        spaces++;
        p->c++;
    }

    return spaces != 0;
}

static bool skip_spaces(Parser *p) {
    while (!is_at_end(p) && ((*p->c) == ' ' || (*p->c) == '\t')) {
        p->c++;
    }

    return true;
}

static bool skip_spaces1(Parser *p) {
    uint32_t spaces = 0;
    while (!is_at_end(p) && ((*p->c) == ' ' || (*p->c) == '\t')) {
        spaces++;
        p->c++;
    }

    return spaces != 0;
}

static bool parse_entry(Parser *p, MtConfigEntry *entry);

static bool parse_object(Parser *p, MtConfigObject *obj) {
    memset(obj, 0, sizeof(*obj));

    if (is_at_end(p) || *p->c != '{') return false;
    p->c++;

    if (!skip_whitespace(p)) return false;

    bool res = true;
    MtConfigEntry entry;
    while (!is_at_end(p) && *p->c != '}' && (res = parse_entry(p, &entry))) {
        mt_array_push(&p->config->alloc, obj->entries, entry);
        if (!skip_whitespace1(p)) {
            return false;
        }
    }

    if (is_at_end(p) || *p->c != '}') {
        return false;
    }
    p->c++;

    return res;
}

static bool parse_number(Parser *p, MtConfigValue *value) {
    bool found_dot = false;

    char str[48];
    char *s = str;
    while (!is_at_end(p) &&
           ((*p->c >= '0' && *p->c <= '9') || *p->c == '-' || *p->c == '.')) {
        if (*p->c == '.') found_dot = true;
        *(s++) = *(p->c++);
    }
    *s = '\0';

    if (found_dot) {
        value->type = MT_CONFIG_VALUE_FLOAT;
        value->f64  = strtod(str, NULL);
    } else {
        value->type = MT_CONFIG_VALUE_INT;
        value->i64  = strtol(str, NULL, 10);
    }

    return true;
}

static bool parse_string(Parser *p, MtConfigValue *value) {
    value->type = MT_CONFIG_VALUE_STRING;

    if (is_at_end(p) || *p->c != '"') return false;
    p->c++;

    mt_str_builder_reset(&p->config->sb);
    while (!is_at_end(p) && *p->c != '"') {
        mt_str_builder_append_char(&p->config->sb, *p->c);
        p->c++;
    }
    value->string = mt_str_builder_build(&p->config->sb, &p->config->bump);

    if (is_at_end(p) || *p->c != '"') return false;
    p->c++;

    return true;
}

static bool parse_multiline_string(Parser *p, MtConfigValue *value) {
    value->type = MT_CONFIG_VALUE_STRING;

    if (is_at_end(p) || strncmp(p->c, "[[", 2) != 0) {
        return false;
    }
    p->c += 2;

    mt_str_builder_reset(&p->config->sb);
    while (!is_at_end(p) && strncmp(p->c, "]]", 2) != 0) {
        mt_str_builder_append_char(&p->config->sb, *p->c);
        p->c++;
    }
    value->string = mt_str_builder_build(&p->config->sb, &p->config->bump);

    if (is_at_end(p) || strncmp(p->c, "]]", 2) != 0) {
        return false;
    }
    p->c += 2;

    return true;
}

static bool parse_entry(Parser *p, MtConfigEntry *entry) {
    memset(entry, 0, sizeof(*entry));

    if (strncmp(p->c, "//", 2) == 0) {
        printf("Hey\n");
        while (!is_at_end(p) && *p->c != '\n') {
            p->c++;
        }
        if (!is_at_end(p)) p->c++;
    }

    if (!skip_whitespace(p)) return false;

    char key[128];
    char *k = key;
    while (!is_at_end(p) && is_alphanum(*p->c)) {
        *(k++) = *(p->c++);
    }
    *k = '\0';

    if (strlen(key) <= 0) return false;

    entry->key = mt_strdup(&p->config->bump, key);

    if (!skip_spaces(p)) return false;

    if (is_at_end(p) || (*p->c) != ':') return false;
    p->c++;

    if (!skip_spaces(p)) return false;

    if (*(p->c) == '{') {
        // Parse object
        entry->value.type = MT_CONFIG_VALUE_OBJECT;
        return parse_object(p, &entry->value.object);
    }

    if ((*(p->c) >= '0' && *(p->c) <= '9') || *p->c == '.' || *p->c == '-') {
        // Parse number
        return parse_number(p, &entry->value);
    }

    if (*(p->c) == '"') {
        return parse_string(p, &entry->value);
    }

    if (strncmp(p->c, "[[", 2) == 0) {
        return parse_multiline_string(p, &entry->value);
    }

    if (strncmp(p->c, "true", 4) == 0) {
        p->c += 4;
        entry->value.type    = MT_CONFIG_VALUE_BOOL;
        entry->value.boolean = true;
        return true;
    }

    if (strncmp(p->c, "false", 4) == 0) {
        p->c += 4;
        entry->value.type    = MT_CONFIG_VALUE_BOOL;
        entry->value.boolean = true;
        return true;
    }

    return false;
}

MtConfig *mt_config_parse(char *input, uint64_t input_size) {
    MtAllocator alloc;
    mt_arena_init(&alloc, 1 << 12);

    MtConfig *config = mt_calloc(&alloc, sizeof(MtConfig));
    config->alloc    = alloc;

    mt_bump_alloc_init(&config->bump, 1 << 12, &config->alloc);
    mt_str_builder_init(&config->sb, &config->alloc);

    Parser parser     = {0};
    parser.input      = input;
    parser.input_size = input_size;
    parser.c          = input;
    parser.config     = config;

    skip_whitespace(&parser);

    bool res = true;
    MtConfigEntry entry;
    while (!is_at_end(&parser) && (res = parse_entry(&parser, &entry))) {
        mt_array_push(&config->alloc, config->root.entries, entry);
        if (!skip_whitespace1(&parser)) {
            return false;
        }
    }

    if (!res) {
        mt_arena_destroy(&alloc);
        return NULL;
    }

    return config;
}

MtConfigObject *mt_config_get_root(MtConfig *config) { return &config->root; }

void mt_config_destroy(MtConfig *config) {
    if (config) {
        mt_str_builder_destroy(&config->sb);
        mt_bump_alloc_destroy(&config->bump);
        mt_arena_destroy(&config->alloc);
    }
}
