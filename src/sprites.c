#include "sprites.h"

static Texture2D LoadSprite(const char* path) {
  Texture2D tex = LoadTexture(path);
  if (IsTextureValid(tex)) {
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
  }
  return tex;
}

void SpritesInit(SpriteAssets* assets) {
  assets->tile_grass = LoadSprite("assets/tile_grass.png");
  assets->tile_dirt = LoadSprite("assets/tile_dirt.png");
  assets->tile_camp = LoadSprite("assets/tile_camp.png");
  assets->tile_grave = LoadSprite("assets/tile_grave.png");
  assets->tree = LoadSprite("assets/tree.png");
  assets->tombstone = LoadSprite("assets/tombstone.png");
  assets->bones = LoadSprite("assets/bones.png");
  assets->player_idle = LoadSprite("assets/player_idle.png");
  assets->player_run = LoadSprite("assets/player_run.png");
  assets->player_attack = LoadSprite("assets/player_attack.png");
  assets->skeleton_idle = LoadSprite("assets/skeleton_idle.png");
  assets->skeleton_run = LoadSprite("assets/skeleton_run.png");
  assets->skeleton_attack = LoadSprite("assets/skeleton_attack.png");
  assets->zombie_idle = LoadSprite("assets/zombie_idle.png");
  assets->zombie_run = LoadSprite("assets/zombie_run.png");
  assets->zombie_attack = LoadSprite("assets/zombie_attack.png");
  assets->lich_idle = LoadSprite("assets/lich_idle.png");
  assets->lich_run = LoadSprite("assets/lich_run.png");
  assets->lich_attack = LoadSprite("assets/lich_attack.png");
  assets->campfire = LoadSprite("assets/campfire.png");
  assets->projectile = LoadSprite("assets/projectile.png");
}

void SpritesUnload(SpriteAssets* assets) {
  UnloadTexture(assets->tile_grass);
  UnloadTexture(assets->tile_dirt);
  UnloadTexture(assets->tile_camp);
  UnloadTexture(assets->tile_grave);
  UnloadTexture(assets->tree);
  UnloadTexture(assets->tombstone);
  UnloadTexture(assets->bones);
  UnloadTexture(assets->player_idle);
  UnloadTexture(assets->player_run);
  UnloadTexture(assets->player_attack);
  UnloadTexture(assets->skeleton_idle);
  UnloadTexture(assets->skeleton_run);
  UnloadTexture(assets->skeleton_attack);
  UnloadTexture(assets->zombie_idle);
  UnloadTexture(assets->zombie_run);
  UnloadTexture(assets->zombie_attack);
  UnloadTexture(assets->lich_idle);
  UnloadTexture(assets->lich_run);
  UnloadTexture(assets->lich_attack);
  UnloadTexture(assets->campfire);
  UnloadTexture(assets->projectile);
}

void AnimationUpdate(Animation* anim, float delta) {
  if (anim->frame_count <= 0) {
    return;
  }
  anim->frame_timer += delta;
  if (anim->frame_timer >= anim->frame_duration) {
    anim->frame_timer -= anim->frame_duration;
    anim->current_frame = (anim->current_frame + 1) % anim->frame_count;
  }
}

void AnimationSet(Animation* anim, int kind, int frame_count, float frame_duration) {
  if (anim->kind != kind) {
    anim->kind = kind;
    anim->frame_count = frame_count;
    anim->frame_duration = frame_duration;
    anim->current_frame = 0;
    anim->frame_timer = 0.0F;
  }
}

Rectangle AnimationFrame(const Animation* anim, int frame_w, int frame_h) {
  return (Rectangle){
      .x = (float)(anim->current_frame * frame_w),
      .y = 0.0F,
      .width = (float)frame_w,
      .height = (float)frame_h,
  };
}
