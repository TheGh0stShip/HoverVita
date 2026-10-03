/*
 * renderer.h - draws a maze with the GPU (desktop GL / vitaGL).
 *
 * The original rasterises in software at 20 fps. To hold 60 fps at 960x544
 * on the Vita we build the static world into vertex batches at load time and
 * draw it as textured polygons. Textures keep their 8-bit palette look with
 * nearest filtering. Placement constants marked TODO(re) are best guesses
 * until the original renderer is reverse engineered.
 */
#ifndef HOVER_RENDERER_H
#define HOVER_RENDERER_H

#include "engine/maze.h"
#include "engine/texture.h"

typedef struct {
    float x, y, z;    /* world units; z up */
    float yaw, pitch; /* radians; yaw 0 = +x */
} Camera;

typedef struct Renderer Renderer;

Renderer *renderer_create(const Maze *maze, const TextureSet *textures);
void renderer_destroy(Renderer *r);
void renderer_draw(Renderer *r, const Camera *cam);

/* Debug overlay: bar of CPU time for the last frame, against the budget. */
void renderer_draw_frame_meter(float frame_ms, float budget_ms);

#endif
