/*
 * HoverVita - entry point and main loop.
 *
 * Milestone 2: fly through the original mazes, rendered with the GPU at
 * 60 fps on top of the game's fixed 20 Hz simulation step (game/timing.h).
 *
 * Controls (Vita / PC):
 *   left stick / WASD       move          right stick / arrows  look
 *   R / PageDown, L / PgUp  up / down     Triangle / E          next maze
 *   Select / Tab            frame meter   Start / Esc           quit
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/log.h"
#include "engine/maze.h"
#include "engine/texture.h"
#include "game/timing.h"
#include "platform/platform.h"
#include "platform/gl.h"
#include "render/renderer.h"

static const struct {
    const char *maze, *textures;
} levels[] = {
    {"mazes/maze1.maz", "mazes/text1.tex"},
    {"mazes/maze2.maz", "mazes/text2.tex"},
    {"mazes/maze3.maz", "mazes/text3.tex"},
};
#define NUM_LEVELS (int)(sizeof(levels) / sizeof(levels[0]))

#define FLY_SPEED 1600.0f /* world units per second */
#define LOOK_SPEED 2.2f   /* radians per second */
#define EYE_HEIGHT 160.0f /* TODO(re): the hovercraft's eye height */

typedef struct {
    Maze maze;
    TextureSet textures;
    Renderer *renderer;
    int loaded;
} Level;

static void level_unload(Level *lv)
{
    if (!lv->loaded)
        return;
    renderer_destroy(lv->renderer);
    maze_free(&lv->maze);
    texture_set_free(&lv->textures);
    memset(lv, 0, sizeof(*lv));
}

static int level_load(Level *lv, int index, Camera *spawn)
{
    level_unload(lv);
    size_t msize, tsize;
    void *mdata = platform_load_file(levels[index].maze, &msize);
    void *tdata = platform_load_file(levels[index].textures, &tsize);
    int ok = mdata && tdata;
    ok = ok && maze_load(&lv->maze, mdata, msize) == 0;
    free(mdata);
    if (ok && texture_set_load(&lv->textures, tdata, tsize) != 0) {
        texture_set_free(&lv->textures); /* owns tdata now */
        maze_free(&lv->maze);
        ok = 0;
    } else if (!ok) {
        free(tdata);
    }
    if (!ok) {
        log_error("could not load %s", levels[index].maze);
        return -1;
    }
    lv->renderer = renderer_create(&lv->maze, &lv->textures);
    lv->loaded = 1;

    const MazeLocation *start = maze_find_location(&lv->maze, "HUMAN_00");
    memset(spawn, 0, sizeof(*spawn));
    spawn->x = start ? start->x : (lv->maze.minx + lv->maze.maxx) / 2.0f;
    spawn->y = start ? start->y : (lv->maze.miny + lv->maze.maxy) / 2.0f;
    spawn->z = EYE_HEIGHT;
    log_info("level %d: %d walls, start at (%.0f, %.0f)", index + 1, lv->maze.nwalls, spawn->x,
             spawn->y);
    return 0;
}

/* One fixed simulation step. Placeholder fly camera until the hovercraft
 * physics are reverse engineered. */
static void sim_step(Camera *cam, const InputState *in)
{
    const float dt = 1.0f / SIM_HZ;
    float lookx = in->rx + !!(in->held & BTN_RIGHT) - !!(in->held & BTN_LEFT);
    float looky = in->ry + !!(in->held & BTN_DOWN) - !!(in->held & BTN_UP);
    cam->yaw -= lookx * LOOK_SPEED * dt;
    cam->pitch -= looky * LOOK_SPEED * dt;
    if (cam->pitch > 1.2f)
        cam->pitch = 1.2f;
    if (cam->pitch < -1.2f)
        cam->pitch = -1.2f;

    float fx = cosf(cam->yaw), fy = sinf(cam->yaw);
    cam->x += (fx * -in->ly + fy * in->lx) * FLY_SPEED * dt;
    cam->y += (fy * -in->ly - fx * in->lx) * FLY_SPEED * dt;
    cam->z += (!!(in->held & BTN_R) - !!(in->held & BTN_L)) * FLY_SPEED * 0.5f * dt;
}

/* --screenshot FILE: render a few frames, save a PPM and exit (testing). */
static void save_screenshot(const char *path)
{
    uint8_t *px = malloc(SCREEN_W * SCREEN_H * 3);
    FILE *f = px ? fopen(path, "wb") : NULL;
    if (f) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, px);
        fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
        for (int y = SCREEN_H - 1; y >= 0; y--) /* GL rows are bottom-up */
            fwrite(px + y * SCREEN_W * 3, 1, SCREEN_W * 3, f);
        fclose(f);
        log_info("screenshot saved to %s", path);
    }
    free(px);
}

static Camera lerp_camera(const Camera *a, const Camera *b, float t)
{
    Camera c;
    c.x = a->x + (b->x - a->x) * t;
    c.y = a->y + (b->y - a->y) * t;
    c.z = a->z + (b->z - a->z) * t;
    c.yaw = a->yaw + (b->yaw - a->yaw) * t;
    c.pitch = a->pitch + (b->pitch - a->pitch) * t;
    return c;
}

int main(int argc, char **argv)
{
    const char *screenshot = NULL;
    int start_level = 0;
    for (int i = 1; i + 1 < argc; i++) {
        if (strcmp(argv[i], "--screenshot") == 0)
            screenshot = argv[++i];
        else if (strcmp(argv[i], "--level") == 0)
            start_level = (atoi(argv[++i]) - 1 + NUM_LEVELS) % NUM_LEVELS;
    }
    if (platform_init() != 0)
        return 1;
    log_info("HoverVita: data directory %s", platform_data_dir());

    Level level;
    memset(&level, 0, sizeof(level));
    int level_index = start_level;
    Camera prev, cur;
    level_load(&level, level_index, &cur);
    prev = cur;

    int show_meter = 1;
    InputState in;
    memset(&in, 0, sizeof(in));
    uint32_t pressed_since_tick = 0;
    uint64_t last = platform_time_us(), acc = 0;
    uint64_t stat_start = last;
    int stat_frames = 0;
    float stat_worst = 0, frame_ms = 0;

    for (;;) {
        uint64_t now = platform_time_us();
        acc += now - last;
        last = now;

        platform_poll_input(&in);
        pressed_since_tick |= in.pressed;
        if (in.quit || (in.pressed & BTN_START))
            break;
        if (in.pressed & BTN_SELECT)
            show_meter = !show_meter;

        /* fixed-rate simulation */
        int steps = 0;
        while (acc >= SIM_DT_US) {
            acc -= SIM_DT_US;
            if (++steps > SIM_MAX_STEPS_PER_FRAME) {
                acc = 0;
                break;
            }
            InputState tick = in;
            tick.pressed = pressed_since_tick;
            pressed_since_tick = 0;
            if (tick.pressed & BTN_TRIANGLE) {
                level_index = (level_index + 1) % NUM_LEVELS;
                level_load(&level, level_index, &cur);
                prev = cur;
                continue;
            }
            prev = cur;
            sim_step(&cur, &tick);
        }

        /* render, interpolated between the last two sim states */
        uint64_t frame_start = platform_time_us();
        Camera view = lerp_camera(&prev, &cur, (float)acc / SIM_DT_US);
        if (level.loaded) {
            renderer_draw(level.renderer, &view);
        } else {
            glClearColor(0.6f, 0.1f, 0.1f, 1); /* no data: red screen */
            glClear(GL_COLOR_BUFFER_BIT);
        }
        if (show_meter)
            renderer_draw_frame_meter(frame_ms, FRAME_BUDGET_MS);
        frame_ms = (platform_time_us() - frame_start) / 1000.0f;
        if (screenshot && stat_frames == 10) {
            save_screenshot(screenshot);
            break;
        }
        platform_swap();

        /* frame stats to the log every 5 s */
        stat_frames++;
        float total_ms = (platform_time_us() - now) / 1000.0f;
        if (total_ms > stat_worst)
            stat_worst = total_ms;
        if (platform_time_us() - stat_start >= 5000000) {
            float secs = (platform_time_us() - stat_start) / 1e6f;
            log_info("%.1f fps, worst frame %.1f ms, cpu %.2f ms", stat_frames / secs, stat_worst,
                     frame_ms);
            stat_start = platform_time_us();
            stat_frames = 0;
            stat_worst = 0;
        }
    }

    level_unload(&level);
    platform_shutdown();
    return 0;
}
