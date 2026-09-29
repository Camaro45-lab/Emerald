#include "global.h"
#include "malloc.h"
#include "data.h"
#include "three_pose.h"

void BuildThreePosePixels(const u8 *src, u8 *dst, unsigned int pose);

struct ThreePoseState
{
    const struct SpriteFrameImage *originalImages;
    const union AnimCmd *const *originalAnims;
    SpriteCallback next;
    struct SpriteFrameImage images[3];
    union AnimCmd commands[5];
    const union AnimCmd *anims[1];
    u8 originalAnim;
    bool8 originalPaused;
    u8 pixels[2][MON_PIC_SIZE];
};

EWRAM_DATA static struct ThreePoseState *sPoseStates[MAX_SPRITES] = {0};

static const union AnimCmd sPoseCommands[] =
{
    ANIMCMD_FRAME(0, 10),
    ANIMCMD_FRAME(1, 12),
    ANIMCMD_FRAME(2, 12),
    ANIMCMD_FRAME(0, 10),
    ANIMCMD_END,
};

void ForgetThreePoseAnimations(void)
{
    u32 i;
    // InitHeap invalidates every old allocation. Never free an old address
    // after the new heap has been initialized.
    for (i = 0; i < MAX_SPRITES; i++)
        sPoseStates[i] = NULL;
}

void ReleaseThreePoseAnimation(struct Sprite *sprite)
{
    u32 id = sprite - gSprites;
    struct ThreePoseState *state;
    if (id >= MAX_SPRITES || sPoseStates[id] == NULL)
        return;
    state = sPoseStates[id];
    CancelSpriteCopiesFrom(state, sizeof(*state));
    sprite->images = state->originalImages;
    sprite->anims = state->originalAnims;
    StartSpriteAnim(sprite, state->originalAnim);
    sprite->animPaused = state->originalPaused;
    sPoseStates[id] = NULL;
    Free(state);
}

static void FinishThreePoseAnimation(struct Sprite *sprite)
{
    struct ThreePoseState *state = sPoseStates[sprite - gSprites];
    SpriteCallback next;
    if (state == NULL)
    {
        sprite->callback = SpriteCallbackDummy;
        return;
    }
    if (!sprite->animEnded)
        return;
    next = state->next;
    ReleaseThreePoseAnimation(sprite);
    sprite->callback = next;
}

void StartThreePoseAnimation(struct Sprite *sprite, SpriteCallback next)
{
    struct ThreePoseState *state;
    const u8 *source;
    u32 i, frame = 0;
    u32 id = sprite - gSprites;

    // Sheet sprites use tile offsets instead of frame-image pointers.
    if (id >= MAX_SPRITES || sprite->usingSheet || sprite->images == NULL)
    {
        sprite->callback = next;
        return;
    }
    ReleaseThreePoseAnimation(sprite);
    // Castform stores weather forms in its four frame slots. Back sprites
    // also select forms through gAnims_MonPic. Never overwrite those slots.
    if (sprite->anims == gMonFrontAnimsPtrTable[SPECIES_CASTFORM]
     || sprite->anims == gAnims_MonPic)
        frame = sprite->anims[sprite->animNum][0].frame.imageValue;
    if (frame >= MAX_MON_PIC_FRAMES || sprite->images[frame].size != MON_PIC_SIZE)
    {
        sprite->callback = next;
        return;
    }
    state = AllocZeroed(sizeof(*state));
    if (state == NULL)
    {
        // Preserve the original playable animation under memory pressure.
        sprite->callback = next;
        return;
    }
    source = sprite->images[frame].data;
    state->originalImages = sprite->images;
    state->originalAnims = sprite->anims;
    state->originalAnim = sprite->animNum;
    state->originalPaused = sprite->animPaused;
    state->next = next;
    state->images[0].data = source;
    for (i = 0; i < 3; i++)
        state->images[i].size = MON_PIC_SIZE;
    for (i = 1; i < 3; i++)
    {
        BuildThreePosePixels(source, state->pixels[i - 1], i);
        state->images[i].data = state->pixels[i - 1];
    }
    for (i = 0; i < ARRAY_COUNT(sPoseCommands); i++)
    {
        state->commands[i] = sPoseCommands[i];
        if (i < 4)
        {
            state->commands[i].frame.hFlip = sprite->hFlip;
            state->commands[i].frame.vFlip = sprite->vFlip;
        }
    }
    state->anims[0] = state->commands;
    sPoseStates[id] = state;
    sprite->images = state->images;
    sprite->anims = state->anims;
    sprite->animPaused = FALSE;
    StartSpriteAnim(sprite, 0);
    sprite->callback = FinishThreePoseAnimation;
}
