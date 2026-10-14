// Exercise the shipped ROM through emulated button presses, using symbols
// only for assertions (not to bypass the real menu or sprite loading).
#include <mgba/core/core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct mCore *core;
static color_t video[240 * 160];
static const char *symbolFile;

static unsigned symbol(const char *name)
{
    FILE *file = fopen(symbolFile, "r");
    char line[512], found[256], type;
    unsigned address;
    if (!file) exit(2);
    while (fgets(line, sizeof(line), file))
        if (sscanf(line, "%x %c %255s", &address, &type, found) == 3 && !strcmp(name, found))
        {
            fclose(file);
            return address;
        }
    fprintf(stderr, "Missing symbol: %s\n", name);
    exit(2);
}

static void screenshot(const char *path)
{
    FILE *file = fopen(path, "wb");
    unsigned i;
    if (!file) exit(2);
    fprintf(file, "P6\n240 160\n255\n");
    for (i = 0; i < 240 * 160; i++)
    {
        unsigned char rgb[] = {video[i] & 255, video[i] >> 8, video[i] >> 16};
        fwrite(rgb, 1, 3, file);
    }
    fclose(file);
}

static void require(int condition, const char *message)
{
    if (!condition)
    {
        screenshot("gallery-failure.ppm");
        fprintf(stderr, "Gallery test failed: %s\n", message);
        exit(1);
    }
}

static void frames(unsigned count, unsigned keys)
{
    core->setKeys(core, keys);
    while (count--) core->runFrame(core);
}

static void tap(unsigned keys, unsigned wait)
{
    // Sprite generation can span hardware frames on the GBA CPU. Give
    // short presses time to be sampled and the resulting picture to settle.
    frames(6, keys);
    frames(wait < 30 ? 30 : wait, 0);
}

static int taskActive(unsigned function)
{
    unsigned i, tasks = symbol("gTasks");
    for (i = 0; i < 16; i++)
        if (core->busRead8(core, tasks + 40*i + 4)
            && (core->busRead32(core, tasks + 40*i) & ~1u) == (function & ~1u))
            return 1;
    return 0;
}

static unsigned spriteHash(void)
{
    unsigned x, y, hash = 2166136261u;
    for (y = 53; y < 117; y++)
        for (x = 146; x < 210; x++)
            hash = (hash ^ video[y*240+x]) * 16777619u;
    return hash;
}

int main(int argc, char **argv)
{
    unsigned i, view, mainAddress, gallery, dex, pic, poses, input, error, battery;
    unsigned hashes[3], id;
    if (argc != 3) return 2;
    symbolFile = argv[2];
    mainAddress = symbol("gMain");
    gallery = symbol("GalleryMain");
    dex = symbol("sGalleryDex");
    pic = symbol("sGallerySprite");
    poses = symbol("sPoseStates");
    input = symbol("Task_HandleMainMenuInput");
    error = symbol("Task_WaitForSaveFileErrorWindow");
    battery = symbol("Task_WaitForBatteryDryErrorWindow");
    core = mCoreFind(argv[1]);
    require(core && core->init(core), "initialize emulator");
    mCoreInitConfig(core, NULL);
    require(mCoreLoadFile(core, argv[1]), "load ROM");
    core->setVideoBuffer(core, video, 240);
    core->reset(core);
    for (i = 0; i < 4800 && !taskActive(input); i++)
    {
        unsigned keys = i % 120 < 2 ? 8 : 0;
        if ((taskActive(error) || taskActive(battery)) && i % 60 < 2) keys = 1;
        frames(1, keys);
    }
    require(taskActive(input), "reach real start menu");
    frames(5, 0);
    screenshot("gallery-start-menu.ppm");
    tap(128, 8); // Down: New Game -> Sprite Gallery
    tap(1, 120);
    require((core->busRead32(core, mainAddress+4) & ~1u) == (gallery & ~1u), "open gallery from menu");
    require(core->busRead16(core, dex) == 1, "starts at Bulbasaur");
    screenshot("gallery-front.ppm");
    for (i = 0; i < 3; i++)
    {
        char filename[48];
        tap(256, 8); // R: hold the next distinct pose
        hashes[i] = spriteHash();
        snprintf(filename, sizeof(filename), "gallery-pose-%u.ppm", (i+1)%3+1);
        screenshot(filename);
    }
    require(hashes[0] != hashes[1] && hashes[1] != hashes[2] && hashes[0] != hashes[2], "three visibly distinct held poses");
    tap(1, 100);
    id = core->busRead16(core, pic);
    require(core->busRead32(core, poses + id*4) == 0, "replay releases temporary frames");
    for (view = 0; view < 2; view++)
    {
        if (view) tap(4, 6);
        for (i = 1; i <= 386; i++)
        {
            require(core->busRead16(core, dex) == i, "species navigation order");
            id = core->busRead16(core, pic);
            require(id < 64, "sprite allocation");
            if (i > 1 || view)
                require(core->busRead32(core, poses + id*4) != 0, "pose allocation survives repeated browsing");
            tap(16, 6);
        }
        require(core->busRead16(core, dex) == 1, "species wraps after Deoxys");
    }
    screenshot("gallery-back.ppm");
    tap(8, 8);
    require(core->busRead8(core, symbol("sGalleryShiny")) == 1, "toggle shiny");
    tap(128, 8);
    require(core->busRead16(core, dex) == 11, "jump forward ten");
    tap(64, 8);
    require(core->busRead16(core, dex) == 1, "jump back ten");
    tap(2, 120); // B during animation, testing cancellation and cleanup.
    require(taskActive(input), "return to main menu");
    for (i = 0; i < 64; i++)
        require(core->busRead32(core, poses+i*4) == 0, "exit releases all pose allocations");
    screenshot("gallery-return-menu.ppm");
    tap(128, 8); // Gallery -> Options
    tap(1, 120);
    tap(2, 120); // Return from Options, still selected
    require(taskActive(input), "options still works");
    tap(64, 8);
    tap(64, 8); // Options -> Gallery -> New Game
    tap(1, 900);
    require(!taskActive(input), "New Game remains reachable");
    screenshot("gallery-new-game.ppm");
    printf("PASS: real menu entry, 386 front + 386 back sprites, 3 held poses, replay cleanup, shiny, jumps, exit, Options and New Game.\n");
    core->deinit(core);
    return 0;
}
