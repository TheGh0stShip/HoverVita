/*
 * timing.h - simulation rate.
 *
 * hover.exe drives the game from timeSetEvent(50 ms) (in 0x4126c0, callback
 * 0x408aa0): one game frame per tick, skipped if the previous one is still
 * running. All gameplay constants assume this 20 Hz step, so we keep it as a
 * fixed timestep and render at the display rate (60 Hz on Vita), interpolating
 * between the last two simulation states.
 */
#ifndef HOVER_TIMING_H
#define HOVER_TIMING_H

#define SIM_HZ 20
#define SIM_DT_US (1000000 / SIM_HZ)
/* After a long stall (loading, suspend), don't try to catch up more than this. */
#define SIM_MAX_STEPS_PER_FRAME 5

#define DISPLAY_HZ 60
#define FRAME_BUDGET_MS (1000.0f / DISPLAY_HZ)

#endif
