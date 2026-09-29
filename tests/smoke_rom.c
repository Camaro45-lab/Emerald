// Headless boot/input smoke test using mGBA's public core API.
// https://github.com/mgba-emu/mgba/blob/0.10.3/include/mgba/core/core.h
#include <mgba/core/core.h>
#include <stdio.h>
#include <stdlib.h>

static color_t pixels[240 * 160];

static unsigned snapshot(const char *name)
{
    FILE *out = fopen(name, "wb");
    unsigned i, different = 0;
    if (!out) exit(2);
    fprintf(out, "P6\n240 160\n255\n");
    for (i = 0; i < 240 * 160; i++)
    {
        unsigned char rgb[3] = {pixels[i] & 255, (pixels[i] >> 8) & 255, (pixels[i] >> 16) & 255};
        fwrite(rgb, 1, 3, out);
        if (pixels[i] != pixels[0]) different++;
    }
    fclose(out);
    return different;
}

int main(int argc, char **argv)
{
    struct mCore *core;
    unsigned frame, nonuniform;
    char title[17] = {0};
    if (argc != 2) return 2;
    core = mCoreFind(argv[1]);
    if (!core || !core->init(core)) return 3;
    mCoreInitConfig(core, NULL);
    if (!mCoreLoadFile(core, argv[1])) return 4;
    core->setVideoBuffer(core, pixels, 240);
    core->reset(core);
    for (frame = 0; frame < 1800; frame++) core->runFrame(core);
    nonuniform = snapshot("smoke-boot.ppm");
    if (nonuniform < 1000) return 5;
    // Tap Start, then A periodically to enter the menu and introduction.
    for (frame = 0; frame < 2400; frame++)
    {
        unsigned keys = frame < 2 ? 8 : (frame > 180 && frame % 90 < 2 ? 1 : 0);
        core->setKeys(core, keys);
        core->runFrame(core);
    }
    nonuniform = snapshot("smoke-intro.ppm");
    if (nonuniform < 1000) return 6;
    core->getGameTitle(core, title);
    printf("ROM smoke: %s; 4200 frames executed; nonblank screens captured.\n", title);
    printf("This confirms boot and frame execution, not a full playthrough.\n");
    core->deinit(core);
    return 0;
}
