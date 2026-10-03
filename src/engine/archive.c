#include "archive.h"

#include <string.h>

#include "log.h"

enum {
    TAG_NULL = 0x0000,
    TAG_BIG_OBJECT = 0x7FFF,
    TAG_CLASS = 0x8000,
    TAG_NEW_CLASS = 0xFFFF,
};

void archive_init(Archive *ar, const void *data, size_t size,
                  const ArchiveClass *classes, void *user)
{
    memset(ar, 0, sizeof(*ar));
    ar->data = data;
    ar->size = size;
    ar->classes = classes;
    ar->user = user;
    ar->nloaded = 1; /* slot 0 == NULL */
}

const uint8_t *archive_bytes(Archive *ar, size_t n)
{
    if (ar->error || n > ar->size - ar->pos) {
        ar->error = 1;
        return NULL;
    }
    const uint8_t *p = ar->data + ar->pos;
    ar->pos += n;
    return p;
}

void archive_skip(Archive *ar, size_t n) { archive_bytes(ar, n); }

/* The data files are little-endian, as is the Vita. */
#define READ_LE(type, ar)                                    \
    do {                                                     \
        type v = 0;                                          \
        const uint8_t *p = archive_bytes(ar, sizeof(type));  \
        if (p)                                               \
            memcpy(&v, p, sizeof(type));                     \
        return v;                                            \
    } while (0)

uint8_t  archive_u8(Archive *ar)  { READ_LE(uint8_t, ar); }
int16_t  archive_i16(Archive *ar) { READ_LE(int16_t, ar); }
uint16_t archive_u16(Archive *ar) { READ_LE(uint16_t, ar); }
int32_t  archive_i32(Archive *ar) { READ_LE(int32_t, ar); }
uint32_t archive_u32(Archive *ar) { READ_LE(uint32_t, ar); }
float    archive_f32(Archive *ar) { READ_LE(float, ar); }

void archive_cstring(Archive *ar, char *buf, size_t bufsize)
{
    uint32_t n = archive_u8(ar);
    if (n == 0xFF)
        n = archive_u16(ar);
    const uint8_t *p = archive_bytes(ar, n);
    size_t copy = (p && bufsize) ? (n < bufsize - 1 ? n : bufsize - 1) : 0;
    if (copy)
        memcpy(buf, p, copy);
    if (bufsize)
        buf[copy] = '\0';
}

uint32_t archive_count(Archive *ar)
{
    uint16_t n = archive_u16(ar);
    return n == 0xFFFF ? archive_u32(ar) : n;
}

static const ArchiveClass *find_class(Archive *ar, const char *name)
{
    for (const ArchiveClass *c = ar->classes; c && c->name; c++)
        if (strcmp(c->name, name) == 0)
            return c;
    return NULL;
}

void *archive_read_object(Archive *ar)
{
    uint32_t tag = archive_u16(ar);
    if (tag == TAG_BIG_OBJECT)
        tag = archive_u32(ar);
    if (ar->error || tag == TAG_NULL)
        return NULL;

    const ArchiveClass *cls;
    if (tag == TAG_NEW_CLASS) {
        char name[ARCHIVE_MAX_CLASSNAME];
        archive_u16(ar); /* schema */
        uint16_t len = archive_u16(ar);
        const uint8_t *p = archive_bytes(ar, len);
        if (!p || len >= sizeof(name) || ar->nloaded >= ARCHIVE_MAX_LOADED) {
            ar->error = 1;
            return NULL;
        }
        memcpy(name, p, len);
        name[len] = '\0';
        cls = find_class(ar, name);
        if (!cls) {
            log_error("archive: no reader for class %s at offset %zu", name, ar->pos);
            ar->error = 1;
            return NULL;
        }
        ar->loaded[ar->nloaded].is_class = 1;
        ar->loaded[ar->nloaded++].cls = cls;
    } else if (tag & TAG_CLASS) {
        uint32_t idx = tag & 0x7FFF;
        if (idx >= (uint32_t)ar->nloaded || !ar->loaded[idx].is_class) {
            ar->error = 1;
            return NULL;
        }
        cls = ar->loaded[idx].cls;
    } else {
        /* back-reference to an already loaded object */
        if (tag >= (uint32_t)ar->nloaded || ar->loaded[tag].is_class) {
            ar->error = 1;
            return NULL;
        }
        return ar->loaded[tag].obj;
    }

    if (ar->nloaded >= ARCHIVE_MAX_LOADED) {
        ar->error = 1;
        return NULL;
    }
    int slot = ar->nloaded++;
    ar->loaded[slot].is_class = 0;
    ar->loaded[slot].obj = cls->read(ar, ar->user);
    return ar->error ? NULL : ar->loaded[slot].obj;
}
