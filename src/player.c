#include "player.h"

#include <math.h>

#include "map.h"
#include "raylib_tester.h"
#include "sprites.h"

static void PlayerScreenToWorld(float screen_x, float screen_y, float cam_x, float cam_y,
                                float* world_x, float* world_y) {
  float rel_x = screen_x - SCREEN_CENTER_X;
  float rel_y = screen_y - SCREEN_CENTER_Y;
  *world_x = (rel_x / (float)ISO_SCALE_X + rel_y / (float)ISO_SCALE_Y) / 2.0F + cam_x;
  *world_y = (rel_y / (float)ISO_SCALE_Y - rel_x / (float)ISO_SCALE_X) / 2.0F + cam_y;
}

void PlayerInit(Player* player) {
  *player = (Player){
      .x = 15.0F,
      .y = 15.0F,
      .hp = PLAYER_MAX_HP,
      .max_hp = PLAYER_MAX_HP,
      .speed = PLAYER_SPEED,
      .attack_range = PLAYER_ATTACK_RANGE,
      .attack_arc = PLAYER_ATTACK_ARC,
      .attack_cooldown = PLAYER_ATTACK_COOLDOWN,
      .damage = PLAYER_DAMAGE,
  };
}

void PlayerHandleInput(GameState* state, float delta) {
  Player* p = &state->player;

  float move_x = 0.0F;
  float move_y = 0.0F;

  if (RltIsKeyDown(KEY_W)) {
    move_x -= 1.0F;
    move_y -= 1.0F;
  }
  if (RltIsKeyDown(KEY_S)) {
    move_x += 1.0F;
    move_y += 1.0F;
  }
  if (RltIsKeyDown(KEY_A)) {
    move_x -= 1.0F;
    move_y += 1.0F;
  }
  if (RltIsKeyDown(KEY_D)) {
    move_x += 1.0F;
    move_y -= 1.0F;
  }

  float len = sqrtf(move_x * move_x + move_y * move_y);
  if (len > 0.0F) {
    move_x /= len;
    move_y /= len;

    float new_x = p->x + move_x * p->speed * delta;
    float new_y = p->y + move_y * p->speed * delta;

    float half = 0.4F;
    bool blocked_x = MapIsSolid(state, new_x - half, p->y - half) ||
                     MapIsSolid(state, new_x + half, p->y - half) ||
                     MapIsSolid(state, new_x - half, p->y + half) ||
                     MapIsSolid(state, new_x + half, p->y + half);

    bool blocked_y = MapIsSolid(state, p->x - half, new_y - half) ||
                     MapIsSolid(state, p->x + half, new_y - half) ||
                     MapIsSolid(state, p->x - half, new_y + half) ||
                     MapIsSolid(state, p->x + half, new_y + half);

    if (!blocked_x) {
      p->x = new_x;
    }
    if (!blocked_y) {
      p->y = new_y;
    }
  }

  p->vx = move_x;
  p->vy = move_y;

  if (RltIsMouseButtonPressed(MOUSE_BUTTON_LEFT) && p->attack_timer <= 0.0F) {
    Vector2 mouse = GetMousePosition();
    float world_mx = 0.0F;
    float world_my = 0.0F;
    PlayerScreenToWorld(mouse.x, mouse.y, state->camera_x, state->camera_y, &world_mx, &world_my);

    float dx = world_mx - p->x;
    float dy = world_my - p->y;
    p->attack_angle = atan2f(dy, dx);
    p->attacking = true;
    p->attack_applied = false;
    p->attack_timer = p->attack_cooldown;
  }
}

void PlayerUpdate(GameState* state, float delta) {
  Player* p = &state->player;

  if (p->attack_timer > 0.0F) {
    p->attack_timer -= delta;
    if (p->attack_timer <= 0.0F) {
      p->attack_timer = 0.0F;
      p->attacking = false;
    }
  }

  if (p->attacking) {
    AnimationSet(&p->anim, ANIM_ATTACK, 4, 0.12F);
  } else if (p->vx != 0.0F || p->vy != 0.0F) {
    AnimationSet(&p->anim, ANIM_RUN, 6, 0.1F);
  } else {
    AnimationSet(&p->anim, ANIM_IDLE, 4, 0.2F);
  }
  AnimationUpdate(&p->anim, delta);

  if (MapIsCamp(state, p->x, p->y)) {
    p->hp += CAMP_REGEN_RATE * delta;
    if (p->hp > p->max_hp) {
      p->hp = p->max_hp;
    }
  }
}
