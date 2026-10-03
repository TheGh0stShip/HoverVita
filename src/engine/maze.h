/*
 * maze.h - Hover! mazes (.maz files, MFC class CMerlinWorld).
 *
 * A maze is a flat 2D map (Doom-style 2.5D): wall segments with a bottom and
 * top height, a 2D BSP over those segments, and named locations for spawns,
 * flags and pods. World units: maze1 spans roughly 0..22500; walls are up to
 * 768 tall. See docs/formats.md.
 */
#ifndef HOVER_MAZE_H
#define HOVER_MAZE_H

#include <stddef.h>
#include <stdint.h>

#define MAZE_NAME_MAX 24

/* CMerlinLine: shared by walls and BSP nodes. */
typedef struct {
    int x1, y1, x2, y2;
    /* derived at load, as CMerlinLine does (hover.exe 0x4010c0) */
    int minx, maxx, miny, maxy;
    int dx, dy;
    int len2;
} MazeLine;

/* Wall texture slots. TODO(re): confirm slot meanings from the renderer. */
enum {
    WALL_TEX_0,
    WALL_TEX_1,
    WALL_TEX_FRONT, /* main face texture (DECAL_xx / WBASE_xx) */
    WALL_TEX_BACK,  /* usually the same as FRONT, or a base texture */
    WALL_TEX_4,
    WALL_TEX_5,
    WALL_TEX_COUNT
};

/* CMerlinStatic */
typedef struct {
    MazeLine line;
    char textures[WALL_TEX_COUNT][MAZE_NAME_MAX];
    int16_t bottom, top;   /* z range of the wall */
    int16_t unk_a8, unk_aa;
    uint8_t flags[3];      /* TODO(re) */
    uint8_t ext_b;
    int16_t ext_s[2];
} MazeWall;

/* CMerlinBSP */
typedef struct {
    MazeLine line;
    int16_t v[5];          /* TODO(re): v[0] index, v[2]/v[3] children? */
    double d[2];
} MazeBspNode;

/* CMerlinLocation */
typedef struct {
    char name[MAZE_NAME_MAX]; /* HUMAN_00, ROBOT_03, FLAG_HUMAN_05, POD_RANDOM_02 ... */
    int16_t x, y, z;
    int16_t radius;           /* TODO(re): 96 for every player spawn */
} MazeLocation;

typedef struct {
    int minx, miny, maxx, maxy;
    int nwalls, nbsp, nlocations;
    MazeWall *walls;
    MazeBspNode *bsp;
    MazeLocation *locations;
} Maze;

int maze_load(Maze *maze, const void *data, size_t size);
void maze_free(Maze *maze);
const MazeLocation *maze_find_location(const Maze *maze, const char *name);

#endif
