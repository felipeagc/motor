#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <motor/base/lexer.h>

int main()
{
    FILE *f = fopen("../src/motor/engine/camera.c", "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *input = malloc(input_size + 1);
    fread(input, 1, input_size, f);
    input[input_size] = '\0';

    fclose(f);

    uint64_t token_count;
    MtToken *tokens = mt_lexer_scan(input, input_size, NULL, &token_count);

    for (uint64_t i = 0; i < token_count; ++i)
    {
        switch (tokens[i].type)
        {
            case MT_TOKEN_NEWLINE:
            {
                break;
            }
            case MT_TOKEN_UNKNOWN:
            {
                printf("%u:\t%.*s\n", tokens[i].type, tokens[i].length, tokens[i].string);
                break;
            }
            default:
            {
                /* printf("%u:\t%.*s\n", tokens[i].type, tokens[i].length, tokens[i].string); */
                break;
            }
        }
    }

    return 0;
}
