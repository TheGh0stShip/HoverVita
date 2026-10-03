#include "renderer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "engine/log.h"
#include "platform/gl.h"
#include "platform/platform.h"

/* TODO(re): world units per texture repeat; the original's mapping is not
 * reverse engineered yet. 768 makes a full-height wall show one repeat. */
#define WALL_UNITS_PER_REPEAT 768.0f
#define FLOOR_UNITS_PER_REPEAT 768.0f
#define FOV_Y_DEGREES 60.0f
#define Z_NEAR 16.0f
#define Z_FAR 40000.0f

typedef struct {
    float x, y, z, u, v;
} Vertex;

typedef struct {
    GLuint tex;
    int has_alpha;
    Vertex *verts;
    int nverts, cap;
} Batch;

struct Renderer {
    const TextureSet *set;
    int nbatches;
    Batch *batches;      /* one per texture in the set, index-aligned */
    int floor_tex, sky_tex;
    float minx, miny, maxx, maxy;
};

/* Expands one mip level of a palettised texture to RGBA. */
static void mip_to_rgba(const TextureSet *set, const TextureMip *m, int level, uint8_t *idx,
                        uint8_t *mask, uint8_t *rgba, int *has_alpha)
{
    size_t n = (size_t)m->width * m->height;
    texture_mip_expand(m, level, idx, mask);
    for (size_t i = 0; i < n; i++) {
        memcpy(rgba + i * 4, set->palette[idx[i]], 3);
        rgba[i * 4 + 3] = mask[i] ? 255 : 0;
        *has_alpha |= !mask[i];
    }
}

static GLuint upload_texture(const TextureSet *set, const Texture *t, int *has_alpha)
{
    const TextureMip *m0 = &t->mips[0];
    size_t n = (size_t)m0->width * m0->height;
    uint8_t *idx = malloc(n), *mask = malloc(n);
    uint8_t *rgba = malloc(n * 4);
    GLuint id = 0;
    *has_alpha = 0;
    if (idx && mask && rgba) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        /* Nearest texel + nearest mip: the original's crisp look, without
         * the shimmer of unfiltered distant walls. */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#ifdef __vita__
        /* TODO: upload the game's own mip chain once vitaGL level uploads are
         * verified on hardware; until then let the GPU build it. */
        mip_to_rgba(set, m0, 0, idx, mask, rgba, has_alpha);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m0->width, m0->height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, rgba);
        glGenerateMipmap(GL_TEXTURE_2D);
#else
        /* The files stop at 4x4, so cap the chain to what they contain. */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, t->nmips - 1);
        for (int level = 0; level < t->nmips; level++) {
            const TextureMip *m = &t->mips[level];
            mip_to_rgba(set, m, level, idx, mask, rgba, has_alpha);
            glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, m->width, m->height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, rgba);
        }
#endif
    }
    free(idx);
    free(mask);
    free(rgba);
    return id;
}

static int find_texture(const TextureSet *set, const char *name)
{
    if (!name[0])
        return -1;
    for (int i = 0; i < set->ntextures; i++)
        if (strcmp(set->textures[i]->name, name) == 0)
            return i;
    return -1;
}

static void push(Batch *b, Vertex v)
{
    if (b->nverts == b->cap) {
        int ncap = b->cap ? b->cap * 2 : 64;
        Vertex *nv = realloc(b->verts, sizeof(Vertex) * ncap);
        if (!nv)
            return;
        b->verts = nv;
        b->cap = ncap;
    }
    b->verts[b->nverts++] = v;
}

static void push_quad(Batch *b, Vertex a, Vertex c, Vertex d, Vertex e)
{
    push(b, a);
    push(b, c);
    push(b, d);
    push(b, a);
    push(b, d);
    push(b, e);
}

static void build_walls(Renderer *r, const Maze *maze)
{
    for (int i = 0; i < maze->nwalls; i++) {
        const MazeWall *w = &maze->walls[i];
        int t = find_texture(r->set, w->textures[WALL_TEX_FRONT]);
        if (t < 0 || w->top <= w->bottom)
            continue;
        const MazeLine *l = &w->line;
        float len = sqrtf((float)l->len2);
        float u1 = len / WALL_UNITS_PER_REPEAT;
        float v0 = 0.0f, v1 = (w->top - w->bottom) / WALL_UNITS_PER_REPEAT;
        Vertex a = {l->x1, l->y1, w->bottom, 0, v1};
        Vertex b = {l->x2, l->y2, w->bottom, u1, v1};
        Vertex c = {l->x2, l->y2, w->top, u1, v0};
        Vertex d = {l->x1, l->y1, w->top, 0, v0};
        /* walls are drawn double-sided (culling is off) */
        push_quad(&r->batches[t], a, b, c, d);
    }
}

Renderer *renderer_create(const Maze *maze, const TextureSet *set)
{
    Renderer *r = calloc(1, sizeof(*r));
    if (!r)
        return NULL;
    r->set = set;
    r->nbatches = set->ntextures;
    r->batches = calloc(set->ntextures ? set->ntextures : 1, sizeof(Batch));
    r->minx = maze->minx;
    r->miny = maze->miny;
    r->maxx = maze->maxx;
    r->maxy = maze->maxy;
    for (int i = 0; i < set->ntextures; i++)
        r->batches[i].tex = upload_texture(set, set->textures[i], &r->batches[i].has_alpha);
    r->floor_tex = find_texture(set, "FBASE_00");
    r->sky_tex = find_texture(set, "BACKGRND");
    build_walls(r, maze);

    if (r->floor_tex >= 0) {
        Batch *b = &r->batches[r->floor_tex];
        float s = FLOOR_UNITS_PER_REPEAT;
        Vertex a = {r->minx, r->miny, 0, r->minx / s, r->miny / s};
        Vertex c = {r->maxx, r->miny, 0, r->maxx / s, r->miny / s};
        Vertex d = {r->maxx, r->maxy, 0, r->maxx / s, r->maxy / s};
        Vertex e = {r->minx, r->maxy, 0, r->minx / s, r->maxy / s};
        push_quad(b, a, c, d, e);
    }

    int total = 0;
    for (int i = 0; i < r->nbatches; i++)
        total += r->batches[i].nverts;
    log_info("renderer: %d triangles", total / 3);
    return r;
}

void renderer_destroy(Renderer *r)
{
    if (!r)
        return;
    for (int i = 0; i < r->nbatches; i++) {
        glDeleteTextures(1, &r->batches[i].tex);
        free(r->batches[i].verts);
    }
    free(r->batches);
    free(r);
}

/* Column-major 4x4 helpers (fixed-function GL, no GLU on the Vita). */
static void mat_perspective(float *m, float fovy_deg, float aspect, float n, float f)
{
    float t = 1.0f / tanf(fovy_deg * (float)M_PI / 360.0f);
    memset(m, 0, 16 * sizeof(float));
    m[0] = t / aspect;
    m[5] = t;
    m[10] = (f + n) / (n - f);
    m[11] = -1.0f;
    m[14] = 2.0f * f * n / (n - f);
}

/* View matrix for a z-up world: x forward at yaw 0. */
static void mat_view(float *m, const Camera *c)
{
    float cy = cosf(c->yaw), sy = sinf(c->yaw);
    float cp = cosf(c->pitch), sp = sinf(c->pitch);
    float f[3] = {cy * cp, sy * cp, sp};     /* forward */
    float s[3] = {sy, -cy, 0};               /* right = forward x up */
    float u[3] = {s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2],
                  s[0] * f[1] - s[1] * f[0]}; /* up = right x forward */
    float e[3] = {c->x, c->y, c->z};
    m[0] = s[0]; m[4] = s[1]; m[8] = s[2];
    m[1] = u[0]; m[5] = u[1]; m[9] = u[2];
    m[2] = -f[0]; m[6] = -f[1]; m[10] = -f[2];
    m[3] = m[7] = m[11] = 0.0f;
    m[12] = -(s[0] * e[0] + s[1] * e[1] + s[2] * e[2]);
    m[13] = -(u[0] * e[0] + u[1] * e[1] + u[2] * e[2]);
    m[14] = f[0] * e[0] + f[1] * e[1] + f[2] * e[2];
    m[15] = 1.0f;
}

static void set_ortho(void)
{
    float m[16] = {2.0f / SCREEN_W, 0, 0, 0, 0, -2.0f / SCREEN_H, 0, 0,
                   0, 0, -1, 0, -1, 1, 0, 1};
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void draw_verts(const Vertex *v, int n)
{
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &v->x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &v->u);
    glDrawArrays(GL_TRIANGLES, 0, n);
}

/* Sky: the BACKGRND texture as a panorama that scrolls with yaw. */
static void draw_sky(const Renderer *r, const Camera *cam)
{
    if (r->sky_tex < 0)
        return;
    set_ortho();
    float repeats = 4.0f; /* TODO(re): panorama repeats per 360 degrees */
    float u0 = -cam->yaw / (2.0f * (float)M_PI) * repeats;
    float u1 = u0 + repeats * (FOV_Y_DEGREES * SCREEN_W / SCREEN_H) / 360.0f;
    /* screen row of the horizon for this pitch (y grows downwards) */
    float half_fov = FOV_Y_DEGREES * (float)M_PI / 360.0f;
    float horizon = SCREEN_H * 0.5f * (1.0f + tanf(cam->pitch) / tanf(half_fov));
    Vertex q[6] = {
        {0, horizon - SCREEN_H, 0, u0, 0}, {SCREEN_W, horizon - SCREEN_H, 0, u1, 0},
        {SCREEN_W, horizon, 0, u1, 1},     {0, horizon - SCREEN_H, 0, u0, 0},
        {SCREEN_W, horizon, 0, u1, 1},     {0, horizon, 0, u0, 1},
    };
    glBindTexture(GL_TEXTURE_2D, r->batches[r->sky_tex].tex);
    draw_verts(q, 6);
}

void renderer_draw(Renderer *r, const Camera *cam)
{
    glViewport(0, 0, SCREEN_W, SCREEN_H);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_CULL_FACE);
    glColor4f(1, 1, 1, 1);

    glDisable(GL_DEPTH_TEST);
    draw_sky(r, cam);

    float m[16];
    glMatrixMode(GL_PROJECTION);
    mat_perspective(m, FOV_Y_DEGREES, (float)SCREEN_W / SCREEN_H, Z_NEAR, Z_FAR);
    glLoadMatrixf(m);
    glMatrixMode(GL_MODELVIEW);
    mat_view(m, cam);
    glLoadMatrixf(m);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    /* Transparent texels are fully clear (sparse spans), so alpha test is
     * enough: no sorting needed. */
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    for (int i = 0; i < r->nbatches; i++) {
        const Batch *b = &r->batches[i];
        if (!b->nverts)
            continue;
        glBindTexture(GL_TEXTURE_2D, b->tex);
        draw_verts(b->verts, b->nverts);
    }
    glDisable(GL_ALPHA_TEST);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void renderer_draw_frame_meter(float frame_ms, float budget_ms)
{
    set_ortho();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    float scale = 200.0f / budget_ms; /* 200 px == one frame budget */
    float w = frame_ms * scale;
    float bar[] = {8, 8, 0, 8 + w, 8, 0, 8 + w, 14, 0, 8, 8, 0, 8 + w, 14, 0, 8, 14, 0};
    float mark[] = {208, 4, 0, 210, 4, 0, 210, 18, 0, 208, 4, 0, 210, 18, 0, 208, 18, 0};
    glVertexPointer(3, GL_FLOAT, 0, bar);
    if (frame_ms < budget_ms * 0.75f)
        glColor4f(0.2f, 0.9f, 0.3f, 1);
    else if (frame_ms < budget_ms)
        glColor4f(0.95f, 0.8f, 0.2f, 1);
    else
        glColor4f(0.95f, 0.2f, 0.2f, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glColor4f(1, 1, 1, 1);
    glVertexPointer(3, GL_FLOAT, 0, mark);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_2D);
}
