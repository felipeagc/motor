#include <motor/base/string_builder.h>

#include <string.h>
#include <assert.h>
#include <motor/base/allocator.h>

void mt_str_builder_init(MtStringBuilder *sb, MtAllocator *alloc) {
    memset(sb, 0, sizeof(*sb));

    sb->capacity = 1 << 14;
    sb->alloc    = alloc;
    sb->buf      = mt_alloc(sb->alloc, sb->capacity);
}

void mt_str_builder_reset(MtStringBuilder *sb) { sb->length = 0; }

void mt_str_builder_append_str(MtStringBuilder *sb, char *str) {
    size_t len = strlen(str);
    for (uint32_t i = 0; i < len; i++)
        sb->buf[sb->length++] = str[i];
    assert(sb->length <= sb->capacity);
}

void mt_str_builder_append_char(MtStringBuilder *sb, char c) {
    sb->buf[sb->length++] = c;
    assert(sb->length <= sb->capacity);
}

char *mt_str_builder_build(MtStringBuilder *sb, MtAllocator *allocator) {
    sb->buf[sb->length] = '\0';
    return mt_strndup(allocator, sb->buf, sb->length + 1);
}

void mt_str_builder_destroy(MtStringBuilder *sb) {
    mt_free(sb->alloc, sb->buf);
}
