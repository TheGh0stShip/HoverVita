#include "maze.h"

#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "log.h"

/* Readers fill the next slot of the matching array; the Maze is the user ptr. */
typedef struct {
    Maze *maze;
    int cap_walls, cap_bsp, cap_locations;
} LoadCtx;

/* Every Merlin Serialize ends with i16 n + n bytes of ignored extension data. */
static void skip_extension(Archive *ar)
{
    int16_t n = archive_i16(ar);
    if (n > 0)
        archive_skip(ar, (size_t)n);
}

/* CMerlinObject::Serialize (0x411b10) */
static void read_object(Archive *ar, char *name, size_t namesize)
{
    archive_cstring(ar, name, namesize);
    skip_extension(ar);
}

/* CMerlinLine::Serialize (0x414740) + derived fields (0x4010c0) */
static void read_line(Archive *ar, MazeLine *l)
{
    char name[MAZE_NAME_MAX];
    read_object(ar, name, sizeof(name));
    l->x1 = archive_i16(ar);
    l->y1 = archive_i16(ar);
    l->x2 = archive_i16(ar);
    l->y2 = archive_i16(ar);
    skip_extension(ar);
    l->minx = l->x1 < l->x2 ? l->x1 : l->x2;
    l->maxx = l->x1 < l->x2 ? l->x2 : l->x1;
    l->miny = l->y1 < l->y2 ? l->y1 : l->y2;
    l->maxy = l->y1 < l->y2 ? l->y2 : l->y1;
    l->dx = l->x2 - l->x1;
    l->dy = l->y2 - l->y1;
    l->len2 = l->dx * l->dx + l->dy * l->dy;
}

static void *grow(void *p, int *cap, int need, size_t elem)
{
    if (need <= *cap)
        return p;
    int ncap = *cap ? *cap * 2 : 64;
    while (ncap < need)
        ncap *= 2;
    void *np = realloc(p, (size_t)ncap * elem);
    if (np)
        *cap = ncap;
    return np;
}

/* CMerlinStatic::Serialize (0x419e00) */
static void *read_static(Archive *ar, void *user)
{
    LoadCtx *ctx = user;
    Maze *m = ctx->maze;
    MazeWall *walls = grow(m->walls, &ctx->cap_walls, m->nwalls + 1, sizeof(MazeWall));
    if (!walls) {
        ar->error = 1;
        return NULL;
    }
    m->walls = walls;
    MazeWall *w = &walls[m->nwalls];
    memset(w, 0, sizeof(*w));
    read_line(ar, &w->line);
    for (int i = 0; i < WALL_TEX_COUNT; i++)
        archive_cstring(ar, w->textures[i], MAZE_NAME_MAX);
    w->bottom = archive_i16(ar);
    w->top = archive_i16(ar);
    w->unk_a8 = archive_i16(ar);
    w->unk_aa = archive_i16(ar);
    for (int i = 0; i < 3; i++)
        w->flags[i] = archive_u8(ar);
    int16_t n = archive_i16(ar);
    if (n >= 5) { /* schema extension added after the first file version */
        w->ext_b = archive_u8(ar);
        w->ext_s[0] = archive_i16(ar);
        w->ext_s[1] = archive_i16(ar);
        n -= 5;
    }
    if (n > 0)
        archive_skip(ar, (size_t)n);
    m->nwalls++;
    return w;
}

/* CMerlinBSP::Serialize (0x41bc40) */
static void *read_bsp(Archive *ar, void *user)
{
    LoadCtx *ctx = user;
    Maze *m = ctx->maze;
    MazeBspNode *bsp = grow(m->bsp, &ctx->cap_bsp, m->nbsp + 1, sizeof(MazeBspNode));
    if (!bsp) {
        ar->error = 1;
        return NULL;
    }
    m->bsp = bsp;
    MazeBspNode *b = &bsp[m->nbsp];
    memset(b, 0, sizeof(*b));
    read_line(ar, &b->line);
    for (int i = 0; i < 5; i++)
        b->v[i] = archive_i16(ar);
    for (int i = 0; i < 2; i++) {
        const uint8_t *p = archive_bytes(ar, 8);
        if (p)
            memcpy(&b->d[i], p, 8); /* little-endian IEEE double on both ends */
    }
    skip_extension(ar);
    m->nbsp++;
    return b;
}

/* CMerlinLocation::Serialize (0x41b940) */
static void *read_location(Archive *ar, void *user)
{
    LoadCtx *ctx = user;
    Maze *m = ctx->maze;
    MazeLocation *locs = grow(m->locations, &ctx->cap_locations, m->nlocations + 1,
                              sizeof(MazeLocation));
    if (!locs) {
        ar->error = 1;
        return NULL;
    }
    m->locations = locs;
    MazeLocation *l = &locs[m->nlocations];
    memset(l, 0, sizeof(*l));
    read_object(ar, l->name, sizeof(l->name));
    l->x = archive_i16(ar);
    l->y = archive_i16(ar);
    l->z = archive_i16(ar);
    l->radius = archive_i16(ar);
    skip_extension(ar);
    m->nlocations++;
    return l;
}

/* CMerlinDynamic::Serialize (0x42cc40). No shipped maze contains any; parse
 * and discard so a modded file doesn't break loading. */
static void *read_dynamic(Archive *ar, void *user)
{
    static MazeLine discard;
    char tex[MAZE_NAME_MAX];
    read_line(ar, &discard);
    archive_cstring(ar, tex, sizeof(tex));
    archive_skip(ar, 5 * 2 + 1);
    skip_extension(ar);
    return &discard;
}

static const ArchiveClass maz_classes[] = {
    {"CMerlinStatic", read_static},
    {"CMerlinBSP", read_bsp},
    {"CMerlinLocation", read_location},
    {"CMerlinDynamic", read_dynamic},
    {NULL, NULL},
};

/* CMerlinWorld::Serialize (0x418dc0): bounds, then four CObArrays
 * (statics, dynamics, locations, BSP nodes). */
int maze_load(Maze *maze, const void *data, size_t size)
{
    memset(maze, 0, sizeof(*maze));
    LoadCtx ctx = {maze, 0, 0, 0};
    Archive *ar = malloc(sizeof(Archive));
    if (!ar)
        return -1;
    archive_init(ar, data, size, maz_classes, &ctx);
    maze->minx = archive_i16(ar);
    maze->miny = archive_i16(ar);
    maze->maxx = archive_i16(ar);
    maze->maxy = archive_i16(ar);
    for (int arr = 0; arr < 4 && !ar->error; arr++) {
        uint32_t n = archive_count(ar);
        for (uint32_t i = 0; i < n && !ar->error; i++)
            archive_read_object(ar);
    }
    int err = ar->error;
    if (err)
        log_error("maze: parse error at offset %zu", ar->pos);
    free(ar);
    if (err)
        maze_free(maze);
    return err ? -1 : 0;
}

void maze_free(Maze *maze)
{
    free(maze->walls);
    free(maze->bsp);
    free(maze->locations);
    memset(maze, 0, sizeof(*maze));
}

const MazeLocation *maze_find_location(const Maze *maze, const char *name)
{
    for (int i = 0; i < maze->nlocations; i++)
        if (strcmp(maze->locations[i].name, name) == 0)
            return &maze->locations[i];
    return NULL;
}
