/*
 * platform.h - the only interface between the engine and the host system.
 *
 *   platform_files.c  data directory + file loading (portable)
 *   platform_pc.c     PC: SDL2 window with an OpenGL context, keyboard/pad
 *   platform_vita.c   Vita: vitaGL, sceCtrl, scePower (no SDL)
 *
 * Rendering goes through OpenGL fixed-function calls that work on both
 * desktop GL and vitaGL; include "platform/gl.h" for them.
 */
#ifndef HOVER_PLATFORM_H
#define HOVER_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#define SCREEN_W 960
#define SCREEN_H 544

/* Directory holding the user's Hover! data (mazes/, sounds/), with a trailing
 * slash. Vita: ux0:data/HoverVita/. PC: $HOVER_DATA or ./hover/. */
const char *platform_data_dir(void);

/* Reads a whole file relative to platform_data_dir(). Matching is
 * case-insensitive and accepts '\' separators, as the original exe used.
 * Returns a malloc'd buffer, or NULL. */
void *platform_load_file(const char *relpath, size_t *size);

enum {
    BTN_UP = 1 << 0,
    BTN_DOWN = 1 << 1,
    BTN_LEFT = 1 << 2,
    BTN_RIGHT = 1 << 3,
    BTN_CROSS = 1 << 4,
    BTN_CIRCLE = 1 << 5,
    BTN_SQUARE = 1 << 6,
    BTN_TRIANGLE = 1 << 7,
    BTN_L = 1 << 8,
    BTN_R = 1 << 9,
    BTN_START = 1 << 10,
    BTN_SELECT = 1 << 11,
};

typedef struct {
    uint32_t held;
    uint32_t pressed;   /* went down since the previous poll */
    float lx, ly;       /* left stick, -1..1, deadzone applied */
    float rx, ry;       /* right stick */
    int quit;           /* window closed (PC) */
} InputState;

/* Opens a SCREEN_W x SCREEN_H display with a GL context and vsync. */
int platform_init(void);
void platform_shutdown(void);
void platform_poll_input(InputState *in);
/* Presents the frame; blocks for vsync, which paces the loop at 60 Hz. */
void platform_swap(void);
/* Monotonic microseconds. */
uint64_t platform_time_us(void);

#endif
