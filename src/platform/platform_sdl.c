#include "platform.h"

#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "../engine/log.h"

void log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, ap);
    va_end(ap);
}

const char *platform_data_dir(void)
{
#ifdef __vita__
    return "ux0:data/HoverVita/";
#else
    static char dir[1024];
    if (!dir[0]) {
        const char *env = getenv("HOVER_DATA");
        snprintf(dir, sizeof(dir), "%s", env && *env ? env : "hover");
        size_t n = strlen(dir);
        if (n + 1 < sizeof(dir) && dir[n - 1] != '/')
            strcat(dir, "/");
    }
    return dir;
#endif
}

/* Resolves each path component case-insensitively ("Mazes\TEXT1.tex"
 * style names from the exe must find mazes/text1.tex). */
static int resolve_path(const char *rel, char *out, size_t outsize)
{
    snprintf(out, outsize, "%s", platform_data_dir());
    char part[256];
    while (*rel) {
        size_t n = strcspn(rel, "/\\");
        if (n >= sizeof(part))
            return -1;
        memcpy(part, rel, n);
        part[n] = '\0';
        rel += n + (rel[n] != '\0');
        if (!n)
            continue;

        size_t len = strlen(out);
        DIR *d = opendir(out);
        int found = 0;
        if (d) {
            struct dirent *e;
            while ((e = readdir(d))) {
                if (SDL_strcasecmp(e->d_name, part) == 0) {
                    snprintf(out + len, outsize - len, "%s%s", e->d_name, *rel ? "/" : "");
                    found = 1;
                    break;
                }
            }
            closedir(d);
        }
        if (!found)
            snprintf(out + len, outsize - len, "%s%s", part, *rel ? "/" : "");
    }
    return 0;
}

void *platform_load_file(const char *relpath, size_t *size)
{
    char path[1024];
    if (resolve_path(relpath, path, sizeof(path)) != 0)
        return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) {
        log_error("cannot open %s", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buf = n > 0 ? malloc((size_t)n) : NULL;
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    if (buf && size)
        *size = (size_t)n;
    return buf;
}
