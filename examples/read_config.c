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
    FILE *f = fopen("../assets/test.config", "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *input = malloc(input_size + 1);
    fread(input, 1, input_size, f);
    input[input_size] = '\0';

    fclose(f);

    MtConfig *config = mt_config_parse(NULL, input, input_size);
    if (!config) {
        printf("Failed to parse\n");
        exit(1);
    }

    print_object(mt_config_get_root(config), 0);

    mt_config_destroy(config);

    free(input);

    return 0;
}
