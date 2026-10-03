/* platform_vita.c - native PS Vita implementation: vitaGL, sceCtrl, scePower. */
#include "platform.h"

#include <stdarg.h>
#include <stdio.h>

#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <vitaGL.h>

#include "../engine/log.h"

static uint32_t prev_held;

static FILE *log_file;

/* Logs go to the debug console and to ux0:data/HoverVita/log.txt (truncated
 * each launch), so frame stats can be read back over FTP. */
static void vlog(const char *level, const char *fmt, va_list ap)
{
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    sceClibPrintf("[%s] %s\n", level, buf);
    if (!log_file)
        log_file = fopen("ux0:data/HoverVita/log.txt", "w");
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", level, buf);
        fflush(log_file);
    }
}

void log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vlog("info", fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vlog("error", fmt, ap);
    va_end(ap);
}

int platform_init(void)
{
    /* Max clocks: the 60 fps budget is 16.6 ms and the game is CPU-light,
     * but there's no reason to leave headroom on the table. */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    /* Keep 8 MB of RAM free for the game; the rest backs vitaGL pools. */
    vglInitExtended(0, SCREEN_W, SCREEN_H, 8 * 1024 * 1024, SCE_GXM_MULTISAMPLE_NONE);
    vglWaitVblankStart(GL_TRUE); /* vsync: lock to 60 Hz */

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    log_info("clocks: cpu %d MHz, gpu %d MHz", scePowerGetArmClockFrequency(),
             scePowerGetGpuClockFrequency());
    return 0;
}

/* vitaGL has no teardown; the process exit releases GXM. */
void platform_shutdown(void) {}

static float stick(unsigned char v)
{
    float f = (v - 128) / 127.0f;
    return (f > -0.2f && f < 0.2f) ? 0.0f : (f < -1.0f ? -1.0f : f);
}

void platform_poll_input(InputState *in)
{
    static const struct {
        uint32_t sce, bit;
    } map[] = {
        {SCE_CTRL_UP, BTN_UP},         {SCE_CTRL_DOWN, BTN_DOWN},
        {SCE_CTRL_LEFT, BTN_LEFT},     {SCE_CTRL_RIGHT, BTN_RIGHT},
        {SCE_CTRL_CROSS, BTN_CROSS},   {SCE_CTRL_CIRCLE, BTN_CIRCLE},
        {SCE_CTRL_SQUARE, BTN_SQUARE}, {SCE_CTRL_TRIANGLE, BTN_TRIANGLE},
        {SCE_CTRL_LTRIGGER, BTN_L},    {SCE_CTRL_RTRIGGER, BTN_R},
        {SCE_CTRL_START, BTN_START},   {SCE_CTRL_SELECT, BTN_SELECT},
    };
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(0, &pad, 1);
    uint32_t held = 0;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (pad.buttons & map[i].sce)
            held |= map[i].bit;
    in->held = held;
    in->pressed = held & ~prev_held;
    prev_held = held;
    in->lx = stick(pad.lx);
    in->ly = stick(pad.ly);
    in->rx = stick(pad.rx);
    in->ry = stick(pad.ry);
    in->quit = 0;
}

void platform_swap(void) { vglSwapBuffers(GL_FALSE); }

uint64_t platform_time_us(void) { return sceKernelGetProcessTimeWide(); }
