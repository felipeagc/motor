#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <motor/arena.h>
#include <motor/config.h>
#include <motor/array.h>

static void print_object(MtConfigObject *obj, uint32_t indent) {
    for (uint32_t i = 0; i < mt_array_size(obj->entries); i++) {
        for (uint32_t j = 0; j < indent; j++) {
            printf("    ");
        }

        MtConfigEntry *entry = &obj->entries[i];
        printf("%s: ", entry->key);
        switch (entry->value.type) {
        case MT_CONFIG_VALUE_STRING: {
            printf("\"%s\"\n", entry->value.string);
        } break;
        case MT_CONFIG_VALUE_BOOL: {
            if (entry->value.boolean) {
                printf("true\n");
            } else {
                printf("false\n");
            }
        } break;
        case MT_CONFIG_VALUE_INT: {
            printf("%ld\n", entry->value.i64);
        } break;
        case MT_CONFIG_VALUE_FLOAT: {
            printf("%lf\n", entry->value.f64);
        } break;
        case MT_CONFIG_VALUE_OBJECT: {
            puts("");
            print_object(&entry->value.object, indent + 1);
        } break;
        default: assert(0);
        }
    }
}

int main() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 1 << 16);

    FILE *f = fopen("../assets/test.config", "r");
    assert(f);

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *input = mt_alloc(&alloc, input_size);
    fread(input, input_size, 1, f);

    fclose(f);

    MtConfig config;
    if (!mt_config_parse(&config, &alloc, input, input_size)) {
        printf("Failed to parse\n");
        exit(1);
    }

    print_object(&config.root, 0);

    mt_config_destroy(&config);

    mt_arena_destroy(&alloc);

    return 0;
}
