#include "global.h"
#include "bg.h"
#include "data.h"
#include "gpu_regs.h"
#include "main.h"
#include "main_menu.h"
#include "menu.h"
#include "palette.h"
#include "pokemon.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "sprite_gallery.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "three_pose.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/rgb.h"

static EWRAM_DATA u16 sGalleryDex = 1;
static EWRAM_DATA u16 sGallerySprite = 0xFFFF;
static EWRAM_DATA bool8 sGalleryFront = TRUE;
static EWRAM_DATA bool8 sGalleryShiny = FALSE;
static EWRAM_DATA u8 sGalleryPose = 0;
static EWRAM_DATA bool8 sGalleryHeld = FALSE;
static EWRAM_DATA bool8 sGalleryLeaving = FALSE;

static const struct BgTemplate sGalleryBg =
{
    .bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .priority = 1
};
static const struct WindowTemplate sGalleryWindows[] =
{
    {0, 1, 1, 28, 18, 0, 1},
    DUMMY_WIN_TEMPLATE
};
static const u16 sGalleryPalette[] = {RGB(3, 5, 8), RGB_WHITE, RGB(12, 16, 19)};
static const u8 sGalleryColors[] = {0, 1, 2};
static const u8 sTitle[] = _("SPRITE GALLERY");
static const u8 sFront[] = _("FRONT");
static const u8 sBack[] = _("BACK");
static const u8 sShiny[] = _("SHINY");
static const u8 sNormal[] = _("NORMAL");
static const u8 sHelp[] = _("LEFT/RIGHT: next\nUP/DOWN: +/-10\nSELECT: front/back\nA: replay\nL/R: hold pose\nSTART: shiny\nB: main menu");
static const u8 sPoseLabel[] = _("POSE ");
static const u8 sThreePoses[] = _("3 POSES");
static const u8 sUnavailable[] = _("Unable to load sprite");

static void GalleryText(const u8 *text, u8 x, u8 y)
{
    AddTextPrinterParameterized3(0, FONT_SMALL, x, y, sGalleryColors, TEXT_SKIP_DRAW, text);
}

static void GalleryDrawText(void)
{
    u8 text[32];
    u8 *end;
    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    GalleryText(sTitle, 0, 0);
    end = ConvertIntToDecimalStringN(text, sGalleryDex, STR_CONV_MODE_LEADING_ZEROS, 3);
    *end++ = CHAR_SPACE;
    StringCopy(end, gSpeciesNames[NationalPokedexNumToSpecies(sGalleryDex)]);
    GalleryText(text, 0, 19);
    GalleryText(sGalleryFront ? sFront : sBack, 0, 33);
    GalleryText(sGalleryShiny ? sShiny : sNormal, 45, 33);
    GalleryText(sHelp, 0, 49);
    end = StringCopy(text, sPoseLabel);
    ConvertIntToDecimalStringN(end, sGalleryPose + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
    GalleryText(sGalleryHeld ? text : sThreePoses, 147, 123);
    if (sGallerySprite == 0xFFFF)
        GalleryText(sUnavailable, 110, 65);
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static void GalleryDestroySprite(void)
{
    if (sGallerySprite != 0xFFFF)
    {
        struct Sprite *sprite = &gSprites[sGallerySprite];
        ReleaseThreePoseAnimation(sprite);
        CancelSpriteCopiesFrom(sprite->images[0].data, MON_PIC_SIZE * MAX_MON_PIC_FRAMES);
        FreeAndDestroyMonPicSprite(sGallerySprite);
        sGallerySprite = 0xFFFF;
    }
}

static void GalleryShowSpecies(void)
{
    GalleryDestroySprite();
    sGallerySprite = CreateMonPicSprite_HandleDeoxys(NationalPokedexNumToSpecies(sGalleryDex),
        sGalleryShiny ? 0 : SHINY_ODDS, 0, sGalleryFront, 178, 85, 0, TAG_NONE);
    sGalleryPose = 0;
    sGalleryHeld = FALSE;
    if (sGallerySprite != 0xFFFF)
        StartThreePoseAnimation(&gSprites[sGallerySprite], SpriteCallbackDummy);
    GalleryDrawText();
}

static void GalleryVBlank(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void GalleryMain(void)
{
    s16 next = sGalleryDex;
    if (!gPaletteFade.active)
    {
        if (sGalleryLeaving)
        {
            SetVBlankCallback(NULL);
            GalleryDestroySprite();
            FreeAllWindowBuffers();
            SetMainCallback2(CB2_ReinitMainMenu);
            return;
        }
        if (JOY_NEW(B_BUTTON))
        {
            sGalleryLeaving = TRUE;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        }
        else if (JOY_REPEAT(DPAD_LEFT)) next--;
        else if (JOY_REPEAT(DPAD_RIGHT)) next++;
        else if (JOY_REPEAT(DPAD_UP)) next -= 10;
        else if (JOY_REPEAT(DPAD_DOWN)) next += 10;
        else if (JOY_NEW(SELECT_BUTTON | START_BUTTON))
        {
            if (JOY_NEW(SELECT_BUTTON)) sGalleryFront = !sGalleryFront;
            else sGalleryShiny = !sGalleryShiny;
            GalleryShowSpecies();
        }
        else if (sGallerySprite != 0xFFFF && JOY_NEW(A_BUTTON | L_BUTTON | R_BUTTON))
        {
            struct Sprite *sprite = &gSprites[sGallerySprite];
            StartThreePoseAnimation(sprite, SpriteCallbackDummy);
            if (JOY_NEW(L_BUTTON | R_BUTTON))
            {
                sGalleryPose = (sGalleryPose + (JOY_NEW(R_BUTTON) ? 1 : 2)) % 3;
                sGalleryHeld = HoldThreePoseFrame(sprite, sGalleryPose);
            }
            else
            {
                sGalleryPose = 0;
                sGalleryHeld = FALSE;
            }
            GalleryDrawText();
        }
        if (next != sGalleryDex)
        {
            if (next < 1) next += NATIONAL_DEX_COUNT;
            if (next > NATIONAL_DEX_COUNT) next -= NATIONAL_DEX_COUNT;
            sGalleryDex = next;
            GalleryShowSpecies();
        }
    }
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

void CB2_InitSpriteGallery(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetAllPicSprites();
    ResetPaletteFade();
    DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
    DmaClear32(3, OAM, OAM_SIZE);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, &sGalleryBg, 1);
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    InitWindows(sGalleryWindows);
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    LoadPalette(sGalleryPalette, 0, sizeof(sGalleryPalette));
    sGalleryDex = 1;
    sGallerySprite = 0xFFFF;
    sGalleryFront = TRUE;
    sGalleryShiny = FALSE;
    sGalleryLeaving = FALSE;
    GalleryShowSpecies();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    SetVBlankCallback(GalleryVBlank);
    SetMainCallback2(GalleryMain);
}
