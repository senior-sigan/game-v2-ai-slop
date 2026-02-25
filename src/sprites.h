#pragma once
#include "game.h"
#include <raylib.h>

#define ANIM_IDLE 0
#define ANIM_RUN 1
#define ANIM_ATTACK 2

typedef struct {
    Texture2D tile_grass;
    Texture2D tile_dirt;
    Texture2D tile_camp;
    Texture2D tile_grave;
    Texture2D tree;
    Texture2D tombstone;
    Texture2D bones;
    Texture2D player_idle;
    Texture2D player_run;
    Texture2D player_attack;
    Texture2D skeleton_idle;
    Texture2D skeleton_run;
    Texture2D skeleton_attack;
    Texture2D zombie_idle;
    Texture2D zombie_run;
    Texture2D zombie_attack;
    Texture2D lich_idle;
    Texture2D lich_run;
    Texture2D lich_attack;
    Texture2D campfire;
    Texture2D projectile;
} SpriteAssets;

void SpritesInit(SpriteAssets* assets);
void SpritesUnload(SpriteAssets* assets);
void AnimationUpdate(Animation* anim, float delta);
void AnimationSet(Animation* anim, int kind, int frame_count,
                  float frame_duration);
Rectangle AnimationFrame(const Animation* anim, int frame_w, int frame_h);
