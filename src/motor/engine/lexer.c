#include <motor/engine/lexer.h>

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

static bool is_at_end(Lexer *l)
{
    return (uint64_t)(l->c - l->input) >= l->input_size || *l->c == '\0';
}

static bool scan_multiline_string(Lexer *l, MtToken *token)
{
    token->type = MT_TOKEN_STRING;

    if (is_at_end(l) || strncmp(l->c, "@{", 2) != 0)
    {
        return false;
    }
    l->c += 2;

    token->string = l->c;
    while (!is_at_end(l) && strncmp(l->c, "}@", 2) != 0)
    {
        l->c++;
    }
    token->length = (size_t)(l->c - token->string);

    if (is_at_end(l) || strncmp(l->c, "}@", 2) != 0)
    {
        return false;
    }
    l->c += 2;

    return true;
}

static bool scan_string(Lexer *l, MtToken *token)
{
    token->type = MT_TOKEN_STRING;

    if (is_at_end(l) || *l->c != '"')
        return false;
    l->c++;

    token->string = l->c;
    while (!is_at_end(l) && *l->c != '"')
    {
        l->c++;
    }
    token->length = (size_t)(l->c - token->string);

    if (is_at_end(l) || *l->c != '"')
        return false;
    l->c++;

    return true;
}

static bool scan_identifier(Lexer *l, MtToken *token)
{
    token->string = l->c;
    while (!is_at_end(l) && is_alphanum(*l->c))
    {
        l->c++;
    }
    token->length = (size_t)(l->c - token->string);

    token->type = MT_TOKEN_IDENT;

    return true;
}

static bool scan_number(Lexer *l, MtToken *token)
{
    bool found_dot = false;

    char str[48];
    char *s = str;
    while (!is_at_end(l) && ((*l->c >= '0' && *l->c <= '9') || *l->c == '-' || *l->c == '.'))
    {
        if (*l->c == '.')
            found_dot = true;
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

    return true;
}

static bool scan_token(Lexer *l)
{
    if (is_at_end(l))
        return true;

    MtToken token = {0};

    switch (*l->c)
    {
        case ' ':
        case '\r':
        case '\t':
        {
            l->c++;
            return true;
        }
        case '\n':
        {
            l->c++;
            token.type = MT_TOKEN_NEWLINE;
            break;
        }
        case ':':
        {
            l->c++;
            token.type = MT_TOKEN_COLON;
            break;
        }
        case '=':
        {
            l->c++;
            token.type = MT_TOKEN_EQUAL;
            break;
        }
        case '{':
        {
            l->c++;
            token.type = MT_TOKEN_LCURLY;
            break;
        }
        case '}':
        {
            l->c++;
            token.type = MT_TOKEN_RCURLY;
            break;
        }
        case '[':
        {
            l->c++;
            token.type = MT_TOKEN_LBRACK;
            break;
        }
        case ']':
        {
            l->c++;
            token.type = MT_TOKEN_RBRACK;
            break;
        }
        case '@':
        {
            if (*(l->c + 1) == '{')
            {
                if (!scan_multiline_string(l, &token))
                {
                    return false;
                }
                break;
            }
            return false;
        }
        case '"':
        {
            if (!scan_string(l, &token))
            {
                return false;
            }
            break;
        }
        case '/':
        {
            if (*(l->c + 1) == '/')
            {
                // Comment
                while (!is_at_end(l) && *l->c != '\n')
                {
                    l->c++;
                }
                return true;
            }

            return false;
        }
        default:
        {
            if (is_alpha(*l->c))
            {
                if (!scan_identifier(l, &token))
                {
                    return false;
                }
                break;
            }
            if (is_numeric(*l->c))
            {
                if (!scan_number(l, &token))
                {
                    return false;
                }
                break;
            }

            return false;
        }
    }

    if (l->tokens == NULL)
    {
        l->token_capacity = 32;
        l->tokens         = mt_alloc(l->alloc, l->token_capacity * sizeof(MtToken));
    }

    if (l->token_capacity <= l->token_count)
    {
        l->token_capacity *= 2;
        l->tokens = mt_realloc(l->alloc, l->tokens, l->token_capacity * sizeof(MtToken));
    }

    l->tokens[l->token_count++] = token;

    return true;
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
        if (!scan_token(&l))
        {
            mt_free(alloc, l.tokens);
            return NULL;
        }
    }

    *token_count = l.token_count;
    return l.tokens;
}
