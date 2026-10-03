/*
 * Loads every .maz file from the user's Hover! data and checks the parse.
 * Exits 77 (CTest "skipped") when the data is not available, e.g. in CI.
 */
#include <stdio.h>
#include <stdlib.h>

#include "engine/maze.h"
#include "platform/platform.h"

static const struct {
    const char *path;
    int nwalls, nbsp, nlocations;
} cases[] = {
    {"mazes/small.maz", 28, 28, 1},
    {"mazes/maze1.maz", 542, 544, 125},
    {"mazes/maze2.maz", 377, 382, 118},
    {"mazes/maze3.maz", 676, 676, 128},
};

int main(void)
{
    int failures = 0, found = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        size_t size;
        void *data = platform_load_file(cases[i].path, &size);
        if (!data)
            continue;
        found++;
        Maze m;
        int ok = maze_load(&m, data, size) == 0 && m.nwalls == cases[i].nwalls &&
                 m.nbsp == cases[i].nbsp && m.nlocations == cases[i].nlocations;
        /* every big maze has the human player's spawn */
        if (ok && cases[i].nlocations > 1)
            ok = maze_find_location(&m, "HUMAN_00") != NULL;
        for (int w = 0; ok && w < m.nwalls; w++)
            ok = m.walls[w].top >= m.walls[w].bottom;
        printf("%-18s %s (%d walls, %d bsp, %d locations)\n", cases[i].path, ok ? "ok" : "FAIL",
               m.nwalls, m.nbsp, m.nlocations);
        failures += !ok;
        maze_free(&m);
        free(data);
    }
    if (!found) {
        printf("no Hover! data found in %s, skipping\n", platform_data_dir());
        return 77;
    }
    return failures ? 1 : 0;
}
