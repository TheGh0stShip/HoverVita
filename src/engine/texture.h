/*
 * texture.h - Hover! texture sets (.tex files, MFC class CMerlinTexture).
 *
 * A .tex file is a 256-colour palette followed by a list of textures. Each
 * texture is a mip chain; each mip stores its opaque pixels column by column
 * as vertical spans, so transparent areas cost nothing. See docs/formats.md.
 */
#ifndef HOVER_TEXTURE_H
#define HOVER_TEXTURE_H

#include <stddef.h>
#include <stdint.h>

#define TEXTURE_NAME_MAX 16
#define TEXTURE_MAX_MIPS 8

typedef struct {
    int16_t top, bottom; /* inclusive, in mip-0 rows (shift by mip level) */
} TextureSpan;

typedef struct {
    int16_t nspans;
    const TextureSpan *spans;
    uint32_t pixel_offset; /* first pixel of this column in TextureMip.pixels */
} TextureColumn;

typedef struct {
    int width, height;
    int shift;               /* log2 of the level-0 size, as stored in the file */
    const uint8_t *pixels;   /* palette indices, column-major, spans only */
    uint32_t npixels;
    TextureColumn *columns;  /* [width] */
} TextureMip;

typedef struct {
    char name[TEXTURE_NAME_MAX];
    uint16_t flags;
    int nmips;
    TextureMip mips[TEXTURE_MAX_MIPS];
} Texture;

typedef struct {
    uint8_t palette[256][3]; /* RGB */
    int ntextures;
    Texture **textures;
    void *file_data;         /* owned; mips point into it */
    void *arena;             /* owned; columns/spans */
} TextureSet;

/* Takes ownership of data (must be malloc'd). Returns 0 on success. */
int texture_set_load(TextureSet *set, void *data, size_t size);
void texture_set_free(TextureSet *set);
const Texture *texture_set_find(const TextureSet *set, const char *name);

/* Expands a mip into a width*height buffer of palette indices plus an
 * optional coverage mask (1 = opaque). Either output may be NULL. */
void texture_mip_expand(const TextureMip *mip, int level, uint8_t *indices, uint8_t *mask);

#endif
