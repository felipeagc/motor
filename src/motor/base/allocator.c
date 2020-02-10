#include <motor/base/allocator.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* #define LOG_ALLOCS */

void *mt_internal_alloc(MtAllocator *alloc, uint64_t size, const char *filename, uint32_t line)
{
    void *ptr;
    if (alloc)
    {
        ptr = alloc->realloc(alloc->inst, 0, size);
    }
    else
    {
        ptr = malloc(size);
    }
#ifdef LOG_ALLOCS
    printf("alloc\t%p\t%s:%u\n", ptr, filename, line);
#endif
    return ptr;
}

void *mt_internal_realloc(
    MtAllocator *alloc, void *ptr, uint64_t size, const char *filename, uint32_t line)
{
    void *new_ptr;
    if (alloc)
    {
        new_ptr = alloc->realloc(alloc->inst, ptr, size);
    }
    else
    {
        new_ptr = realloc(ptr, size);
    }
#ifdef LOG_ALLOCS
    printf("realloc\t%p -> %p\t%s:%u\n", ptr, new_ptr, filename, line);
#endif
    return new_ptr;
}

void mt_internal_free(MtAllocator *alloc, void *ptr, const char *filename, uint32_t line)
{
#ifdef LOG_ALLOCS
    printf("free\t%p\t%s:%u\n", ptr, filename, line);
#endif
    if (alloc)
    {
        alloc->realloc(alloc->inst, ptr, 0);
    }
    else
    {
        free(ptr);
    }
}

char *mt_strdup(MtAllocator *alloc, const char *str)
{
    uint64_t len = strlen(str);
    char *s = mt_alloc(alloc, len + 1);
    memcpy(s, str, len + 1);
    return s;
}

char *mt_strndup(MtAllocator *alloc, const char *str, uint64_t num_bytes)
{
    char *s = mt_alloc(alloc, num_bytes);
    memcpy(s, str, num_bytes);
    s[num_bytes - 1] = '\0';
    return s;
}

char *mt_strcat(MtAllocator *alloc, char *dest, const char *src)
{
    uint64_t dest_len = strlen(dest);
    uint64_t src_len = strlen(src);
    dest = mt_realloc(alloc, dest, dest_len + src_len + 1);

    char *c = &dest[dest_len];
    for (uint64_t i = 0; i < src_len; i++)
    {
        *(c++) = src[i];
    }
    *c = '\0';

    return dest;
}
