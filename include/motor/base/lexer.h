#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;

typedef enum MtTokenType {
    MT_TOKEN_UNKNOWN,

    MT_TOKEN_NEWLINE,

    MT_TOKEN_LCURLY,
    MT_TOKEN_RCURLY,
    MT_TOKEN_LBRACK,
    MT_TOKEN_RBRACK,
    MT_TOKEN_LPAREN,
    MT_TOKEN_RPAREN,

    MT_TOKEN_MINUS,
    MT_TOKEN_DOT,
    MT_TOKEN_COMMA,
    MT_TOKEN_COLON,
    MT_TOKEN_SEMICOLON,
    MT_TOKEN_EQUAL,
    MT_TOKEN_ASTERISK,

    MT_TOKEN_IDENT,
    MT_TOKEN_STRING,
    MT_TOKEN_FLOAT,
    MT_TOKEN_INT,
} MtTokenType;

typedef struct MtToken
{
    MtTokenType type;
    uint32_t length;
    const char *string;
    union
    {
        int64_t i64;
        double f64;
    };
} MtToken;

MT_BASE_API MtToken *
mt_lexer_scan(const char *input, uint64_t input_size, MtAllocator *alloc, uint64_t *token_count);

#ifdef __cplusplus
}
#endif
