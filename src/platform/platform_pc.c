/* platform_pc.c - SDL2 + OpenGL implementation for the PC development build. */
#include "platform.h"

#include <stdarg.h>
#include <stdio.h>

#include <SDL.h>

#include "../engine/log.h"
#include "gl.h"

static SDL_Window *window;
static SDL_GLContext context;
static SDL_GameController *pad;
static uint32_t prev_held;

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

int platform_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        log_error("SDL_Init: %s", SDL_GetError());
        return -1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window = SDL_CreateWindow("HoverVita", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL);
    if (!window || !(context = SDL_GL_CreateContext(window))) {
        log_error("SDL window/GL: %s", SDL_GetError());
        return -1;
    }
    /* vsync paces the PC build at the monitor rate, like the Vita's 60 Hz */
    SDL_GL_SetSwapInterval(1);
    log_info("GL: %s / %s", (const char *)glGetString(GL_VENDOR),
             (const char *)glGetString(GL_RENDERER));
    for (int i = 0; i < SDL_NumJoysticks() && !pad; i++)
        if (SDL_IsGameController(i))
            pad = SDL_GameControllerOpen(i);
    return 0;
}

void platform_shutdown(void)
{
    if (context)
        SDL_GL_DeleteContext(context);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

static float axis(SDL_GameControllerAxis a)
{
    float v = pad ? SDL_GameControllerGetAxis(pad, a) / 32767.0f : 0.0f;
    return (v > -0.2f && v < 0.2f) ? 0.0f : v;
}

void platform_poll_input(InputState *in)
{
    SDL_Event e;
    in->quit = 0;
    while (SDL_PollEvent(&e))
        if (e.type == SDL_QUIT)
            in->quit = 1;

    static const struct {
        SDL_Scancode key;
        SDL_GameControllerButton button;
        uint32_t bit;
    } map[] = {
        {SDL_SCANCODE_UP, SDL_CONTROLLER_BUTTON_DPAD_UP, BTN_UP},
        {SDL_SCANCODE_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN, BTN_DOWN},
        {SDL_SCANCODE_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT, BTN_LEFT},
        {SDL_SCANCODE_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, BTN_RIGHT},
        {SDL_SCANCODE_SPACE, SDL_CONTROLLER_BUTTON_A, BTN_CROSS},
        {SDL_SCANCODE_LSHIFT, SDL_CONTROLLER_BUTTON_B, BTN_CIRCLE},
        {SDL_SCANCODE_Q, SDL_CONTROLLER_BUTTON_X, BTN_SQUARE},
        {SDL_SCANCODE_E, SDL_CONTROLLER_BUTTON_Y, BTN_TRIANGLE},
        {SDL_SCANCODE_PAGEUP, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, BTN_L},
        {SDL_SCANCODE_PAGEDOWN, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, BTN_R},
        {SDL_SCANCODE_ESCAPE, SDL_CONTROLLER_BUTTON_START, BTN_START},
        {SDL_SCANCODE_TAB, SDL_CONTROLLER_BUTTON_BACK, BTN_SELECT},
    };
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    uint32_t held = 0;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (keys[map[i].key] || (pad && SDL_GameControllerGetButton(pad, map[i].button)))
            held |= map[i].bit;
    in->held = held;
    in->pressed = held & ~prev_held;
    prev_held = held;

    /* WASD doubles as the left stick on keyboard */
    in->lx = axis(SDL_CONTROLLER_AXIS_LEFTX) + keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A];
    in->ly = axis(SDL_CONTROLLER_AXIS_LEFTY) + keys[SDL_SCANCODE_S] - keys[SDL_SCANCODE_W];
    in->rx = axis(SDL_CONTROLLER_AXIS_RIGHTX);
    in->ry = axis(SDL_CONTROLLER_AXIS_RIGHTY);
}

void platform_swap(void) { SDL_GL_SwapWindow(window); }

uint64_t platform_time_us(void)
{
    return SDL_GetPerformanceCounter() * 1000000ull / SDL_GetPerformanceFrequency();
}
