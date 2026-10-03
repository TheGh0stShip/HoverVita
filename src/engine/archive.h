/*
 * archive.h - reader for MFC CArchive streams (the container used by every
 * Hover! data file: .maz, .tex).
 *
 * Only loading is supported. Objects are dispatched by MFC class name to a
 * reader callback; see archive_read_object().
 */
#ifndef HOVER_ARCHIVE_H
#define HOVER_ARCHIVE_H

#include <stddef.h>
#include <stdint.h>

#define ARCHIVE_MAX_LOADED 4096
#define ARCHIVE_MAX_CLASSNAME 32

typedef struct Archive Archive;

/* Reads one object of a known class. Returns the object, or NULL on error. */
typedef void *(*ArchiveClassReader)(Archive *ar, void *user);

typedef struct {
    const char *name;
    ArchiveClassReader read;
} ArchiveClass;

struct Archive {
    const uint8_t *data;
    size_t size;
    size_t pos;
    int error; /* sticky: set on any out-of-bounds read or format error */

    const ArchiveClass *classes; /* terminated by {NULL, NULL} */
    void *user;

    /* MFC load map: index 0 is NULL, then classes and objects in read order */
    int nloaded;
    struct {
        int is_class;
        const ArchiveClass *cls;
        void *obj;
    } loaded[ARCHIVE_MAX_LOADED];
};

void archive_init(Archive *ar, const void *data, size_t size,
                  const ArchiveClass *classes, void *user);

uint8_t  archive_u8(Archive *ar);
int16_t  archive_i16(Archive *ar);
uint16_t archive_u16(Archive *ar);
int32_t  archive_i32(Archive *ar);
uint32_t archive_u32(Archive *ar);
float    archive_f32(Archive *ar);
/* Returns a pointer into the archive buffer (no copy), or NULL on overrun. */
const uint8_t *archive_bytes(Archive *ar, size_t n);
void     archive_skip(Archive *ar, size_t n);
/* CString: length-prefixed. Copies into buf (always NUL terminated). */
void     archive_cstring(Archive *ar, char *buf, size_t bufsize);
/* CArchive::ReadCount */
uint32_t archive_count(Archive *ar);
/* CArchive::ReadObject */
void    *archive_read_object(Archive *ar);

#endif
