/* stdio logger for tests (the real ones live in the platform backends) */
#include <stdarg.h>
#include <stdio.h>

#include "engine/log.h"

void log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    putchar('\n');
    va_end(ap);
}

void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}
