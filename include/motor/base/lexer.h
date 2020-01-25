#pragma once

#include <stdint.h>

typedef struct MtAllocator MtAllocator;

typedef enum MtTokenType
{
    MT_TOKEN_NEWLINE = 1,
    MT_TOKEN_COLON,
    MT_TOKEN_SEMICOLON,
    MT_TOKEN_EQUAL,

    MT_TOKEN_LCURLY,
    MT_TOKEN_RCURLY,

    MT_TOKEN_LBRACK,
    MT_TOKEN_RBRACK,

    MT_TOKEN_IDENT,
    MT_TOKEN_STRING,
    MT_TOKEN_FLOAT,
    MT_TOKEN_INT,
} MtTokenType;

typedef struct MtToken
{
    MtTokenType type;
    union {
        struct
        {
            const char *string;
            uint32_t length;
        };
        int64_t i64;
        double f64;
    };
} MtToken;

MtToken *
mt_lexer_scan(const char *input, uint64_t input_size, MtAllocator *alloc, uint64_t *token_count);
