#include "enemy.h"

#include <math.h>

#include "map.h"
#include "sprites.h"

static void InitEnemy(Enemy* e, EnemyType type, float x, float y) {
  e->type = type;
  e->x = x;
  e->y = y;
  e->spawn_x = x;
  e->spawn_y = y;
  e->alive = true;
  e->state = AI_IDLE;
  e->attack_timer = 0.0F;
  e->respawn_timer = 0.0F;
  e->patrol_target_x = x;
  e->patrol_target_y = y;
  e->anim = (Animation){.frame_count = 4, .frame_duration = 0.2F};

  switch (type) {
    case ENEMY_SKELETON:
      e->hp = 30.0F;
      e->max_hp = 30.0F;
      e->speed = 3.5F;
      e->damage = 5;
      e->aggro_radius = 8.0F;
      e->attack_radius = 1.0F;
      e->attack_cooldown = 1.0F;
      break;
    case ENEMY_ZOMBIE:
      e->hp = 80.0F;
      e->max_hp = 80.0F;
      e->speed = 1.5F;
      e->damage = 12;
      e->aggro_radius = 6.0F;
      e->attack_radius = 1.2F;
      e->attack_cooldown = 1.5F;
      break;
    case ENEMY_LICH:
      e->hp = 50.0F;
      e->max_hp = 50.0F;
      e->speed = 2.5F;
      e->damage = 20;
      e->aggro_radius = 10.0F;
      e->attack_radius = 1.5F;
      e->attack_cooldown = 2.0F;
      break;
  }
}

static void NudgeOffSolid(const GameState* state, float* x, float* y) {
  if (!MapIsSolid(state, *x, *y)) {
    return;
  }
  if (!MapIsSolid(state, *x + 1.0F, *y)) {
    *x += 1.0F;
    return;
  }
  if (!MapIsSolid(state, *x, *y + 1.0F)) {
    *y += 1.0F;
    return;
  }
  if (!MapIsSolid(state, *x - 1.0F, *y)) {
    *x -= 1.0F;
    return;
  }
  if (!MapIsSolid(state, *x, *y - 1.0F)) {
    *y -= 1.0F;
    return;
  }
}

void EnemiesInit(GameState* state) {
  int idx = 0;
  float cx = (float)(MAP_WIDTH / 2);
  float cy = (float)(MAP_HEIGHT / 2);

  for (int i = 0; i < 8; i++) {
    float angle = (float)i * (2.0F * PI / 8.0F);
    float radius = 9.0F + (float)(i % 2);
    float x = cx + cosf(angle) * radius;
    float y = cy + sinf(angle) * radius;
    NudgeOffSolid(state, &x, &y);
    InitEnemy(&state->enemies[idx], ENEMY_SKELETON, x, y);
    idx++;
  }

  for (int i = 0; i < 8; i++) {
    float angle = (float)i * (2.0F * PI / 8.0F) + PI / 8.0F;
    float radius = 10.0F + (float)(i % 2);
    float x = cx + cosf(angle) * radius;
    float y = cy + sinf(angle) * radius;
    NudgeOffSolid(state, &x, &y);
    InitEnemy(&state->enemies[idx], ENEMY_ZOMBIE, x, y);
    idx++;
  }

  for (int i = 0; i < 4; i++) {
    float angle = (float)i * (2.0F * PI / 4.0F) + PI / 4.0F;
    float radius = 11.0F;
    float x = cx + cosf(angle) * radius;
    float y = cy + sinf(angle) * radius;
    NudgeOffSolid(state, &x, &y);
    InitEnemy(&state->enemies[idx], ENEMY_LICH, x, y);
    idx++;
  }

  state->enemy_count = 0;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (state->enemies[i].alive) {
      state->enemy_count++;
    }
  }
}

static void UpdateDeadEnemy(Enemy* e, float delta) {
  e->respawn_timer -= delta;
  if (e->respawn_timer <= 0.0F) {
    e->alive = true;
    e->hp = e->max_hp;
    e->x = e->spawn_x;
    e->y = e->spawn_y;
    e->state = AI_IDLE;
    e->attack_timer = 0.0F;
    e->anim = (Animation){.frame_count = 4, .frame_duration = 0.2F};
  }
}

static void UpdateAiIdle(Enemy* e, float dist, float delta, int index) {
  if (dist < e->aggro_radius) {
    e->state = AI_CHASE;
    return;
  }

  e->attack_timer += delta;
  if (e->attack_timer < 3.0F) {
    return;
  }

  e->attack_timer = 0.0F;
  float ptdx = e->patrol_target_x - e->spawn_x;
  float ptdy = e->patrol_target_y - e->spawn_y;
  float patrol_angle = (float)index * 0.7854F;
  if (fabsf(ptdx) > 0.1F || fabsf(ptdy) > 0.1F) {
    patrol_angle = atan2f(ptdy, ptdx) + 2.3F;
  }
  e->patrol_target_x = e->spawn_x + cosf(patrol_angle) * 3.0F;
  e->patrol_target_y = e->spawn_y + sinf(patrol_angle) * 3.0F;
  if (e->patrol_target_x < 0.5F) {
    e->patrol_target_x = 0.5F;
  }
  if (e->patrol_target_x > (float)MAP_WIDTH - 0.5F) {
    e->patrol_target_x = (float)MAP_WIDTH - 0.5F;
  }
  if (e->patrol_target_y < 0.5F) {
    e->patrol_target_y = 0.5F;
  }
  if (e->patrol_target_y > (float)MAP_HEIGHT - 0.5F) {
    e->patrol_target_y = (float)MAP_HEIGHT - 0.5F;
  }
  e->state = AI_PATROL;
}

static void UpdateAiPatrol(const GameState* state, Enemy* e, float dist, float delta) {
  if (dist < e->aggro_radius) {
    e->state = AI_CHASE;
    return;
  }

  float pdx = e->patrol_target_x - e->x;
  float pdy = e->patrol_target_y - e->y;
  float pdist = sqrtf(pdx * pdx + pdy * pdy);

  if (pdist < 0.2F) {
    e->state = AI_IDLE;
    e->attack_timer = 0.0F;
    return;
  }

  float move_speed = e->speed * 0.5F * delta;
  float nx = pdx / pdist;
  float ny = pdy / pdist;
  float new_x = e->x + nx * move_speed;
  float new_y = e->y + ny * move_speed;
  if (!MapIsSolid(state, new_x, new_y)) {
    e->x = new_x;
    e->y = new_y;
  }
}

static void UpdateAiChase(const GameState* state, Enemy* e, float dx, float dy, float dist,
                          float delta) {
  if (dist > e->aggro_radius * 1.5F) {
    e->state = AI_IDLE;
    e->attack_timer = 0.0F;
    return;
  }
  if (dist <= e->attack_radius) {
    e->state = AI_ATTACK;
    e->attack_timer = 0.0F;
    return;
  }

  float move_speed = e->speed * delta;
  float nx = dx / dist;
  float ny = dy / dist;
  float new_x = e->x + nx * move_speed;
  float new_y = e->y + ny * move_speed;

  if (!MapIsCamp(state, new_x, new_y) && !MapIsSolid(state, new_x, new_y)) {
    e->x = new_x;
    e->y = new_y;
  }
}

static void UpdateAiAttack(Enemy* e, Player* p, float dist, float delta) {
  e->attack_timer -= delta;
  if (e->attack_timer <= 0.0F) {
    p->hp -= (float)e->damage;
    e->attack_timer = e->attack_cooldown;
  }
  if (dist > e->attack_radius) {
    e->state = AI_CHASE;
  }
}

static void UpdateEnemyAnimation(Enemy* e, float delta) {
  switch (e->state) {
    case AI_ATTACK:
      AnimationSet(&e->anim, ANIM_ATTACK, 4, 0.12F);
      break;
    case AI_CHASE:
    case AI_PATROL:
      AnimationSet(&e->anim, ANIM_RUN, 6, 0.1F);
      break;
    default:
      AnimationSet(&e->anim, ANIM_IDLE, 4, 0.2F);
      break;
  }
  AnimationUpdate(&e->anim, delta);
}

void EnemiesUpdate(GameState* state, float delta) {
  Player* p = &state->player;

  for (int i = 0; i < MAX_ENEMIES; i++) {
    Enemy* e = &state->enemies[i];

    if (!e->alive) {
      UpdateDeadEnemy(e, delta);
      continue;
    }

    float dx = p->x - e->x;
    float dy = p->y - e->y;
    float dist = sqrtf(dx * dx + dy * dy);

    switch (e->state) {
      case AI_IDLE:
        UpdateAiIdle(e, dist, delta, i);
        break;
      case AI_PATROL:
        UpdateAiPatrol(state, e, dist, delta);
        break;
      case AI_CHASE:
        UpdateAiChase(state, e, dx, dy, dist, delta);
        break;
      case AI_ATTACK:
        UpdateAiAttack(e, p, dist, delta);
        break;
    }

    UpdateEnemyAnimation(e, delta);
  }

  state->enemy_count = 0;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (state->enemies[i].alive) {
      state->enemy_count++;
    }
  }
}
