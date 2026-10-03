/* gl.h - OpenGL headers for the current platform (desktop GL or vitaGL). */
#ifndef HOVER_GL_H
#define HOVER_GL_H

#ifdef __vita__
#include <vitaGL.h>
#else
#include <SDL_opengl.h>
#endif

#endif
