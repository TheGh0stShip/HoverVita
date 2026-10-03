#include "texture.h"

#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "log.h"

/* Simple bump allocator for the many small column/span arrays. */
typedef struct {
    uint8_t *base;
    size_t used, cap;
} Arena;

static void *arena_alloc(Arena *a, size_t n)
{
    n = (n + 7) & ~(size_t)7;
    if (a->used + n > a->cap)
        return NULL;
    void *p = a->base + a->used;
    a->used += n;
    return p;
}

typedef struct {
    Arena arena;
} LoadCtx;

/* CMerlinObject::Serialize - common base of every Merlin class. */
static void read_merlin_object(Archive *ar, char *name, size_t namesize)
{
    archive_cstring(ar, name, namesize);
    archive_u16(ar); /* TODO(re): meaning unknown, always 0 so far */
}

/* CMerlinTexture::Serialize, load branch (hover.exe 0x412980). The original
 * can drop the largest mips to save memory; we always keep all of them. */
static void *read_texture(Archive *ar, void *user)
{
    LoadCtx *ctx = user;
    Texture *t = arena_alloc(&ctx->arena, sizeof(*t));
    if (!t) {
        ar->error = 1;
        return NULL;
    }
    memset(t, 0, sizeof(*t));
    read_merlin_object(ar, t->name, sizeof(t->name));
    t->flags = archive_u16(ar);
    int nmips = archive_i16(ar);
    if (nmips < 0 || nmips > TEXTURE_MAX_MIPS) {
        log_error("texture %s: bad mip count %d", t->name, nmips);
        ar->error = 1;
        return NULL;
    }
    t->nmips = nmips;

    for (int level = 0; level < nmips && !ar->error; level++) {
        TextureMip *m = &t->mips[level];
        m->width = archive_i16(ar);
        archive_i16(ar); /* width - 1 */
        m->height = archive_i16(ar);
        archive_i16(ar); /* height - 1 */
        m->shift = archive_i16(ar);
        m->npixels = archive_u32(ar);
        m->pixels = archive_bytes(ar, m->npixels);
        archive_u32(ar); /* total span count across all columns */

        if (m->width <= 0 || m->width > 4096) {
            ar->error = 1;
            break;
        }
        m->columns = arena_alloc(&ctx->arena, sizeof(TextureColumn) * m->width);
        if (!m->columns) {
            ar->error = 1;
            break;
        }
        uint32_t offset = 0;
        for (int x = 0; x < m->width && !ar->error; x++) {
            TextureColumn *c = &m->columns[x];
            c->nspans = archive_i16(ar);
            c->pixel_offset = offset;
            if (c->nspans < 0) {
                ar->error = 1;
                break;
            }
            TextureSpan *spans = arena_alloc(&ctx->arena, sizeof(TextureSpan) * (c->nspans + 1));
            if (!spans) {
                ar->error = 1;
                break;
            }
            for (int s = 0; s < c->nspans; s++) {
                spans[s].top = archive_i16(ar);
                spans[s].bottom = archive_i16(ar);
                offset += (spans[s].bottom >> level) - (spans[s].top >> level) + 1;
            }
            c->spans = spans;
        }
        if (offset > m->npixels) {
            log_error("texture %s mip %d: spans need %u pixels, have %u",
                      t->name, level, offset, m->npixels);
            ar->error = 1;
        }
        /* trailing per-mip block, purpose unknown */
        archive_skip(ar, (uint16_t)archive_i16(ar));
    }
    return t;
}

static const ArchiveClass tex_classes[] = {
    {"CMerlinTexture", read_texture},
    {NULL, NULL},
};

int texture_set_load(TextureSet *set, void *data, size_t size)
{
    memset(set, 0, sizeof(*set));
    set->file_data = data;

    /* Columns + spans take well under the file size; size it generously. */
    LoadCtx ctx;
    ctx.arena.cap = size + 64 * 1024;
    ctx.arena.used = 0;
    ctx.arena.base = malloc(ctx.arena.cap);
    set->arena = ctx.arena.base;
    if (!ctx.arena.base)
        return -1;

    Archive *ar = malloc(sizeof(Archive));
    if (!ar)
        return -1;
    archive_init(ar, data, size, tex_classes, &ctx);

    for (int i = 0; i < 256; i++) {
        const uint8_t *q = archive_bytes(ar, 4); /* RGBQUAD: B, G, R, reserved */
        if (!q)
            break;
        set->palette[i][0] = q[2];
        set->palette[i][1] = q[1];
        set->palette[i][2] = q[0];
    }

    uint32_t n = archive_count(ar);
    set->textures = calloc(n ? n : 1, sizeof(Texture *));
    for (uint32_t i = 0; i < n && !ar->error && set->textures; i++)
        set->textures[set->ntextures++] = archive_read_object(ar);

    int err = ar->error || !set->textures;
    if (err)
        log_error("texture set: parse error at offset %zu", ar->pos);
    free(ar);
    return err ? -1 : 0;
}

void texture_set_free(TextureSet *set)
{
    free(set->textures);
    free(set->arena);
    free(set->file_data);
    memset(set, 0, sizeof(*set));
}

const Texture *texture_set_find(const TextureSet *set, const char *name)
{
    for (int i = 0; i < set->ntextures; i++)
        if (strcmp(set->textures[i]->name, name) == 0)
            return set->textures[i];
    return NULL;
}

void texture_mip_expand(const TextureMip *m, int level, uint8_t *indices, uint8_t *mask)
{
    size_t n = (size_t)m->width * m->height;
    if (indices)
        memset(indices, 0, n);
    if (mask)
        memset(mask, 0, n);
    for (int x = 0; x < m->width; x++) {
        const TextureColumn *c = &m->columns[x];
        const uint8_t *src = m->pixels + c->pixel_offset;
        for (int s = 0; s < c->nspans; s++) {
            int y0 = c->spans[s].top >> level, y1 = c->spans[s].bottom >> level;
            for (int y = y0; y <= y1; y++, src++) {
                if (y < 0 || y >= m->height)
                    continue;
                if (indices)
                    indices[y * m->width + x] = *src;
                if (mask)
                    mask[y * m->width + x] = 1;
            }
        }
    }
}
