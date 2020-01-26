#include <motor/base/lexer.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <motor/base/allocator.h>

typedef struct Lexer
{
    MtAllocator *alloc;

    const char *input;
    uint64_t input_size;
    const char *c; // current character

    MtToken *tokens;
    size_t token_capacity;
    size_t token_count;
} Lexer;

static inline bool is_whitespace(char c)
{
    return c == ' ' || c == '\t';
}

static inline bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool is_alphanum(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static inline bool is_numeric(char c)
{
    return (c >= '0' && c <= '9');
}

static inline bool is_at_end(Lexer *l)
{
    return (uint64_t)(l->c - l->input) >= l->input_size || *l->c == '\0';
}

static void skip_whitespace(Lexer *l)
{
    while (is_whitespace(l->c[0]))
    {
        l->c++;
    }

    if (l->c[0] == '/' && l->c[1] == '/')
    {
        // Comment
        while (!is_at_end(l) && *l->c != '\n')
        {
            l->c++;
        }
    }
}

static void scan_multiline_string(Lexer *l, MtToken *token)
{
    token->type = MT_TOKEN_STRING;

    token->string = l->c;
    while (!is_at_end(l) && strncmp(l->c, "}@", 2) != 0)
    {
        l->c++;
    }
    token->length = (size_t)(l->c - token->string);

    if (is_at_end(l) || strncmp(l->c, "}@", 2) != 0)
    {
        return;
    }
    l->c += 2;
}

static void scan_string(Lexer *l, MtToken *token)
{
    token->type = MT_TOKEN_STRING;

    token->string = l->c;
    while (!is_at_end(l) && *l->c != '"')
    {
        l->c++;
    }
    token->length = (size_t)(l->c - token->string);

    if (is_at_end(l) || *l->c != '"')
        return;
    l->c++;
}

static void scan_identifier(Lexer *l, MtToken *token)
{
    token->string = l->c;
    while (!is_at_end(l) && is_alphanum(*l->c))
    {
        l->c++;
    }
    token->length = (size_t)(l->c - token->string);

    token->type = MT_TOKEN_IDENT;
}

static void scan_number(Lexer *l, MtToken *token)
{
    bool found_dot = false;

    char str[48];
    char *s = str;
    while (!is_at_end(l) &&
           ((l->c[0] >= '0' && l->c[0] <= '9') || l->c[0] == '-' || l->c[0] == '.'))
    {
        if (l->c[0] == '.')
            found_dot = true;
        *(s++) = *(l->c++);
    }
    if (l->c[0] == 'f')
    {
        *(s++) = *(l->c++);
    }
    *s = '\0';

    if (found_dot)
    {
        token->type = MT_TOKEN_FLOAT;
        token->f64  = strtod(str, NULL);
    }
    else
    {
        token->type = MT_TOKEN_INT;
        token->i64  = strtol(str, NULL, 10);
    }

    token->length = (size_t)(l->c - token->string);
}

static MtToken scan_token(Lexer *l)
{
    MtToken token = {0};
    token.string  = l->c;
    token.length  = 1;

    const char *c = l->c;
    l->c++;

    switch (c[0])
    {
        case '\r':
        case '\n': token.type = MT_TOKEN_NEWLINE; break;
        case ':': token.type = MT_TOKEN_COLON; break;
        case ';': token.type = MT_TOKEN_SEMICOLON; break;
        case ',': token.type = MT_TOKEN_COMMA; break;
        case '=': token.type = MT_TOKEN_EQUAL; break;
        case '*': token.type = MT_TOKEN_ASTERISK; break;
        case '{': token.type = MT_TOKEN_LCURLY; break;
        case '}': token.type = MT_TOKEN_RCURLY; break;
        case '[': token.type = MT_TOKEN_LBRACK; break;
        case ']': token.type = MT_TOKEN_RBRACK; break;
        case '(': token.type = MT_TOKEN_LPAREN; break;
        case ')': token.type = MT_TOKEN_RPAREN; break;
        case '-':
        {
            if (is_numeric(c[1]))
            {
                l->c--;
                scan_number(l, &token);
                break;
            }
            token.type = MT_TOKEN_MINUS;
            break;
        }
        case '.':
        {
            if (is_numeric(c[1]))
            {
                l->c--;
                scan_number(l, &token);
                break;
            }
            token.type = MT_TOKEN_DOT;
            break;
        }
        case '@':
        {
            if (c[1] == '{')
            {
                l->c++;
                scan_multiline_string(l, &token);
                break;
            }
            break;
        }
        case '"':
        {
            scan_string(l, &token);
            break;
        }
        default:
        {
            if (is_alpha(c[0]))
            {
                l->c--;
                scan_identifier(l, &token);
                break;
            }

            if (is_numeric(c[0]))
            {
                l->c--;
                scan_number(l, &token);
                break;
            }

            token.type = MT_TOKEN_UNKNOWN;
            break;
        }
    }

    return token;
}

MtToken *
mt_lexer_scan(const char *input, uint64_t input_size, MtAllocator *alloc, uint64_t *token_count)
{
    Lexer l      = {0};
    l.alloc      = alloc;
    l.input      = input;
    l.input_size = input_size;
    l.c          = input;

    while (!is_at_end(&l))
    {
        skip_whitespace(&l);

        if (is_at_end(&l))
        {
            break;
        }

        MtToken token = scan_token(&l);

        if (l.tokens == NULL)
        {
            l.token_capacity = 32;
            l.tokens         = mt_alloc(l.alloc, l.token_capacity * sizeof(MtToken));
        }

        if (l.token_capacity <= l.token_count)
        {
            l.token_capacity *= 2;
            l.tokens = mt_realloc(l.alloc, l.tokens, l.token_capacity * sizeof(MtToken));
        }

        l.tokens[l.token_count++] = token;
    }

    *token_count = l.token_count;
    return l.tokens;
}
