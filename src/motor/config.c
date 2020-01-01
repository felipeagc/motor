#include "../../include/motor/config.h"

#include "../../include/motor/arena.h"
#include "../../include/motor/bump_alloc.h"
#include "../../include/motor/array.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct MtConfig {
    MtAllocator *alloc;
    MtAllocator bump;
    MtConfigObject root;
};

typedef struct Parser {
    char *input;
    uint64_t input_size;
    char *c;
    MtConfig *config;
    MtStringBuilder sb;
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

static bool parse_object_entries(Parser *p, MtConfigObject *obj) {
    memset(obj, 0, sizeof(*obj));
    mt_hash_init(&obj->map, 11, p->config->alloc);

    bool res = true;
    MtConfigEntry entry;
    while (!is_at_end(p) && *p->c != '}' && (res = parse_entry(p, &entry))) {
        uint64_t key_hash = mt_hash_str(entry.key);
        if (!mt_hash_get_ptr(&obj->map, key_hash)) {
            MtConfigEntry *entry_ptr =
                mt_array_push(p->config->alloc, obj->entries, entry);
            mt_hash_set_ptr(&obj->map, key_hash, entry_ptr);
        } else {
            return false;
        }

        if (!skip_whitespace1(p)) {
            return false;
        }
    }

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

    mt_str_builder_reset(&p->sb);
    while (!is_at_end(p) && *p->c != '"') {
        mt_str_builder_append_char(&p->sb, *p->c);
        p->c++;
    }
    value->string = mt_str_builder_build(&p->sb, &p->config->bump);

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

    mt_str_builder_reset(&p->sb);
    while (!is_at_end(p) && strncmp(p->c, "]]", 2) != 0) {
        mt_str_builder_append_char(&p->sb, *p->c);
        p->c++;
    }
    value->string = mt_str_builder_build(&p->sb, &p->config->bump);

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

        if (is_at_end(p) || *p->c != '{') return false;
        p->c++;

        if (!skip_whitespace(p)) return false;

        bool res = parse_object_entries(p, &entry->value.object);

        if (is_at_end(p) || *p->c != '}') {
            return false;
        }
        p->c++;

        return res;
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

static void destroy_object(MtConfig *config, MtConfigObject *obj) {
    for (uint32_t i = 0; i < mt_array_size(obj->entries); i++) {
        if (obj->entries[i].value.type == MT_CONFIG_VALUE_OBJECT) {
            destroy_object(config, &obj->entries[i].value.object);
        }
    }
    mt_hash_destroy(&obj->map);
    mt_array_free(config->alloc, obj->entries);
}

MtConfig *
mt_config_parse(MtAllocator *alloc, char *input, uint64_t input_size) {
    MtConfig *config = mt_alloc(alloc, sizeof(MtConfig));
    memset(config, 0, sizeof(*config));

    config->alloc = alloc;

    mt_bump_alloc_init(&config->bump, 1 << 12, config->alloc);

    MtStringBuilder sb;

    Parser parser     = {0};
    parser.input      = input;
    parser.input_size = input_size;
    parser.c          = input;
    parser.config     = config;

    mt_str_builder_init(&parser.sb, config->alloc);

    skip_whitespace(&parser);

    bool res = parse_object_entries(&parser, &config->root);

    mt_str_builder_destroy(&parser.sb);

    if (!res) {
        mt_free(alloc, config);
        return NULL;
    }

    return config;
}

MtConfigObject *mt_config_get_root(MtConfig *config) { return &config->root; }

void mt_config_destroy(MtConfig *config) {
    if (config) {
        destroy_object(config, &config->root);
        mt_bump_alloc_destroy(&config->bump);
        mt_free(config->alloc, config);
    }
}
