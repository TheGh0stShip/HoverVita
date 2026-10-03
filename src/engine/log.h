/* log.h - logging, implemented by the platform layer. */
#ifndef HOVER_LOG_H
#define HOVER_LOG_H

void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);

#endif
