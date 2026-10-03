/*
 * Loads every .tex file from the user's Hover! data and checks the parse.
 * Exits 77 (CTest "skipped") when the data is not available, e.g. in CI.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/texture.h"
#include "platform/platform.h"

static const struct {
    const char *path;
    int ntextures;
} cases[] = {
    {"mazes/small.tex", 21},
    {"mazes/text1.tex", 151},
    {"mazes/text2.tex", 154},
    {"mazes/text3.tex", 151},
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
        TextureSet set;
        int err = texture_set_load(&set, data, size);
        int ok = !err && set.ntextures == cases[i].ntextures;
        for (int t = 0; ok && t < set.ntextures; t++) {
            const Texture *tex = set.textures[t];
            ok = tex && tex->nmips > 0 && tex->mips[0].width > 0;
            /* each mip halves the previous one */
            for (int m = 1; ok && m < tex->nmips; m++)
                ok = tex->mips[m].width == tex->mips[m - 1].width / 2;
        }
        printf("%-18s %s (%d textures)\n", cases[i].path, ok ? "ok" : "FAIL", set.ntextures);
        failures += !ok;
        texture_set_free(&set);
    }
    if (!found) {
        printf("no Hover! data found in %s, skipping\n", platform_data_dir());
        return 77;
    }
    return failures ? 1 : 0;
}
