/*
 * platform.h - the only interface between the engine and the host system.
 * Implemented once on top of SDL2 (platform_sdl.c), which covers both the
 * PS Vita build and the PC development build.
 */
#ifndef HOVER_PLATFORM_H
#define HOVER_PLATFORM_H

#include <stddef.h>

/* Directory holding the user's Hover! data (mazes/, sounds/), with a trailing
 * slash. Vita: ux0:data/HoverVita/. PC: $HOVER_DATA or ./hover/. */
const char *platform_data_dir(void);

/* Reads a whole file relative to platform_data_dir(). Matching is
 * case-insensitive, since the original game ran on a case-insensitive FS.
 * Returns a malloc'd buffer, or NULL. */
void *platform_load_file(const char *relpath, size_t *size);

#endif
