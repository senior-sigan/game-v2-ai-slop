#include "combat.h"

#include <math.h>
#include <raylib.h>

void CombatUpdate(GameState* state) {
  Player* p = &state->player;

  if (!p->attacking) {
    p->attack_applied = false;
    return;
  }

  if (p->attack_applied) {
    return;
  }
  p->attack_applied = true;

  for (int i = 0; i < MAX_ENEMIES; i++) {
    Enemy* e = &state->enemies[i];
    if (!e->alive) {
      continue;
    }

    float dx = e->x - p->x;
    float dy = e->y - p->y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > p->attack_range) {
      continue;
    }

    float angle = atan2f(dy, dx);
    float diff = angle - p->attack_angle;
    while (diff > PI) {
      diff -= 2.0F * PI;
    }
    while (diff < -PI) {
      diff += 2.0F * PI;
    }
    if (fabsf(diff) > p->attack_arc / 2.0F) {
      continue;
    }

    e->hp -= (float)p->damage;
    if (e->hp <= 0.0F) {
      e->alive = false;
      switch (e->type) {
        case ENEMY_SKELETON:
          e->respawn_timer = 10.0F;
          break;
        case ENEMY_ZOMBIE:
          e->respawn_timer = 15.0F;
          break;
        case ENEMY_LICH:
          e->respawn_timer = 20.0F;
          break;
      }
      p->kills++;
    }
  }
}
