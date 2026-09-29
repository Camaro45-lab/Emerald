#ifndef GUARD_THREE_POSE_H
#define GUARD_THREE_POSE_H

#include "sprite.h"

void StartThreePoseAnimation(struct Sprite *sprite, SpriteCallback next);
void ReleaseThreePoseAnimation(struct Sprite *sprite);
void ForgetThreePoseAnimations(void);
void CancelSpriteCopiesFrom(const void *start, u32 size);

#endif
