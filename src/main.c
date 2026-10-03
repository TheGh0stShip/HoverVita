/*
 * HoverVita - entry point.
 *
 * Milestone 1: a texture viewer that proves the data pipeline end to end
 * (user data on the memory card -> CArchive parser -> palette -> screen).
 * It will be replaced by the game loop as the engine is reimplemented.
 *
 * Controls: Left/Right (D-pad or keys) = texture, Up/Down = mip level,
 *           L/R or PageUp/PageDown = texture file, Start/Esc = quit.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "engine/log.h"
#include "engine/texture.h"
#include "platform/platform.h"

#define SCREEN_W 960
#define SCREEN_H 544

static const char *const tex_files[] = {
    "mazes/text1.tex", "mazes/text2.tex", "mazes/text3.tex", "mazes/small.tex",
};
#define NUM_TEX_FILES (int)(sizeof(tex_files) / sizeof(tex_files[0]))

typedef struct {
    SDL_Renderer *ren;
    SDL_Texture *tex;
    TextureSet set;
    int loaded;
    int file, index, level;
} Viewer;

static void viewer_load_file(Viewer *v)
{
    if (v->loaded)
        texture_set_free(&v->set);
    v->loaded = 0;
    v->index = v->level = 0;
    size_t size;
    void *data = platform_load_file(tex_files[v->file], &size);
    if (data && texture_set_load(&v->set, data, size) == 0 && v->set.ntextures > 0) {
        v->loaded = 1;
        log_info("%s: %d textures", tex_files[v->file], v->set.ntextures);
    } else if (data) {
        texture_set_free(&v->set);
    }
}

static void viewer_upload(Viewer *v)
{
    if (v->tex)
        SDL_DestroyTexture(v->tex);
    v->tex = NULL;
    if (!v->loaded)
        return;

    const Texture *t = v->set.textures[v->index];
    if (v->level >= t->nmips)
        v->level = t->nmips - 1;
    const TextureMip *m = &t->mips[v->level];
    size_t n = (size_t)m->width * m->height;
    uint8_t *idx = malloc(n), *mask = malloc(n);
    uint32_t *rgba = malloc(n * 4);
    if (idx && mask && rgba) {
        texture_mip_expand(m, v->level, idx, mask);
        for (size_t i = 0; i < n; i++) {
            const uint8_t *c = v->set.palette[idx[i]];
            rgba[i] = mask[i] ? (0xFFu << 24 | c[2] << 16 | c[1] << 8 | c[0]) : 0;
        }
        v->tex = SDL_CreateTexture(v->ren, SDL_PIXELFORMAT_ABGR8888,
                                   SDL_TEXTUREACCESS_STATIC, m->width, m->height);
        if (v->tex) {
            SDL_SetTextureBlendMode(v->tex, SDL_BLENDMODE_BLEND);
            SDL_UpdateTexture(v->tex, NULL, rgba, m->width * 4);
        }
        log_info("[%d/%d] %s mip %d (%dx%d)", v->index + 1, v->set.ntextures,
                 t->name, v->level, m->width, m->height);
    }
    free(idx);
    free(mask);
    free(rgba);
}

static void draw(Viewer *v)
{
    SDL_SetRenderDrawColor(v->ren, 32, 32, 48, 255);
    SDL_RenderClear(v->ren);
    if (v->tex) {
        int w, h;
        SDL_QueryTexture(v->tex, NULL, NULL, &w, &h);
        /* fit to screen, integer-free scaling, keep aspect */
        float s = SDL_min((SCREEN_W - 32.0f) / w, (SCREEN_H - 32.0f) / h);
        SDL_Rect dst = {(int)(SCREEN_W - w * s) / 2, (int)(SCREEN_H - h * s) / 2,
                        (int)(w * s), (int)(h * s)};
        SDL_RenderCopy(v->ren, v->tex, NULL, &dst);
    } else {
        /* no data: red bar so the problem is visible on hardware */
        SDL_Rect r = {0, SCREEN_H / 2 - 8, SCREEN_W, 16};
        SDL_SetRenderDrawColor(v->ren, 200, 40, 40, 255);
        SDL_RenderFillRect(v->ren, &r);
    }
    SDL_RenderPresent(v->ren);
}

typedef enum { CMD_NONE, CMD_QUIT, CMD_PREV, CMD_NEXT, CMD_UP, CMD_DOWN, CMD_PREV_FILE, CMD_NEXT_FILE } Command;

static Command translate(const SDL_Event *e)
{
    if (e->type == SDL_QUIT)
        return CMD_QUIT;
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
        case SDLK_ESCAPE: return CMD_QUIT;
        case SDLK_LEFT: return CMD_PREV;
        case SDLK_RIGHT: return CMD_NEXT;
        case SDLK_UP: return CMD_UP;
        case SDLK_DOWN: return CMD_DOWN;
        case SDLK_PAGEUP: return CMD_PREV_FILE;
        case SDLK_PAGEDOWN: return CMD_NEXT_FILE;
        }
    }
    if (e->type == SDL_CONTROLLERBUTTONDOWN) {
        switch (e->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_START: return CMD_QUIT;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return CMD_PREV;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return CMD_NEXT;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return CMD_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return CMD_DOWN;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return CMD_PREV_FILE;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return CMD_NEXT_FILE;
        }
    }
    return CMD_NONE;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("HoverVita", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       SCREEN_W, SCREEN_H, 0);
    Viewer v;
    memset(&v, 0, sizeof(v));
    v.ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (!v.ren) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        if (SDL_IsGameController(i))
            SDL_GameControllerOpen(i);

    log_info("HoverVita: data directory %s", platform_data_dir());
    viewer_load_file(&v);
    viewer_upload(&v);

    for (int running = 1; running;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            int n = v.loaded ? v.set.ntextures : 1;
            switch (translate(&e)) {
            case CMD_QUIT: running = 0; break;
            case CMD_PREV: v.index = (v.index + n - 1) % n; v.level = 0; viewer_upload(&v); break;
            case CMD_NEXT: v.index = (v.index + 1) % n; v.level = 0; viewer_upload(&v); break;
            case CMD_UP: if (v.level > 0) v.level--; viewer_upload(&v); break;
            case CMD_DOWN: v.level++; viewer_upload(&v); break;
            case CMD_PREV_FILE:
                v.file = (v.file + NUM_TEX_FILES - 1) % NUM_TEX_FILES;
                viewer_load_file(&v);
                viewer_upload(&v);
                break;
            case CMD_NEXT_FILE:
                v.file = (v.file + 1) % NUM_TEX_FILES;
                viewer_load_file(&v);
                viewer_upload(&v);
                break;
            default: break;
            }
        }
        draw(&v);
    }

    if (v.loaded)
        texture_set_free(&v.set);
    SDL_Quit();
    return 0;
}
