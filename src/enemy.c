#include "enemy.h"
#include "map.h"
#include "sprites.h"
#include <math.h>

#define PI 3.14159265358979323846f

static void InitEnemy(Enemy* e, EnemyType type, float x, float y) {
    e->type = type;
    e->x = x;
    e->y = y;
    e->spawn_x = x;
    e->spawn_y = y;
    e->alive = true;
    e->state = AI_IDLE;
    e->attack_timer = 0.0f;
    e->respawn_timer = 0.0f;
    e->patrol_target_x = x;
    e->patrol_target_y = y;
    e->anim = (Animation){.frame_count = 4, .frame_duration = 0.2f};

    switch (type) {
    case ENEMY_SKELETON:
        e->hp = 30.0f;
        e->max_hp = 30.0f;
        e->speed = 3.5f;
        e->damage = 5;
        e->aggro_radius = 8.0f;
        e->attack_radius = 1.0f;
        e->attack_cooldown = 1.0f;
        break;
    case ENEMY_ZOMBIE:
        e->hp = 80.0f;
        e->max_hp = 80.0f;
        e->speed = 1.5f;
        e->damage = 12;
        e->aggro_radius = 6.0f;
        e->attack_radius = 1.2f;
        e->attack_cooldown = 1.5f;
        break;
    case ENEMY_LICH:
        e->hp = 50.0f;
        e->max_hp = 50.0f;
        e->speed = 2.5f;
        e->damage = 20;
        e->aggro_radius = 10.0f;
        e->attack_radius = 1.5f;
        e->attack_cooldown = 2.0f;
        break;
    }
}

static void NudgeOffSolid(const GameState* state, float* x, float* y) {
    if (!MapIsSolid(state, *x, *y)) return;
    if (!MapIsSolid(state, *x + 1.0f, *y)) { *x += 1.0f; return; }
    if (!MapIsSolid(state, *x, *y + 1.0f)) { *y += 1.0f; return; }
    if (!MapIsSolid(state, *x - 1.0f, *y)) { *x -= 1.0f; return; }
    if (!MapIsSolid(state, *x, *y - 1.0f)) { *y -= 1.0f; return; }
}

void EnemiesInit(GameState* state) {
    int idx = 0;
    float cx = (float)(MAP_WIDTH / 2);
    float cy = (float)(MAP_HEIGHT / 2);

    for (int i = 0; i < 8; i++) {
        float angle = (float)i * (2.0f * PI / 8.0f);
        float radius = 9.0f + (float)(i % 2);
        float x = cx + cosf(angle) * radius;
        float y = cy + sinf(angle) * radius;
        NudgeOffSolid(state, &x, &y);
        InitEnemy(&state->enemies[idx], ENEMY_SKELETON, x, y);
        idx++;
    }

    for (int i = 0; i < 8; i++) {
        float angle = (float)i * (2.0f * PI / 8.0f) + PI / 8.0f;
        float radius = 10.0f + (float)(i % 2);
        float x = cx + cosf(angle) * radius;
        float y = cy + sinf(angle) * radius;
        NudgeOffSolid(state, &x, &y);
        InitEnemy(&state->enemies[idx], ENEMY_ZOMBIE, x, y);
        idx++;
    }

    for (int i = 0; i < 4; i++) {
        float angle = (float)i * (2.0f * PI / 4.0f) + PI / 4.0f;
        float radius = 11.0f;
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

void EnemiesUpdate(GameState* state, float delta) {
    Player* p = &state->player;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy* e = &state->enemies[i];

        if (!e->alive) {
            e->respawn_timer -= delta;
            if (e->respawn_timer <= 0.0f) {
                e->alive = true;
                e->hp = e->max_hp;
                e->x = e->spawn_x;
                e->y = e->spawn_y;
                e->state = AI_IDLE;
                e->attack_timer = 0.0f;
                e->anim = (Animation){
                    .frame_count = 4, .frame_duration = 0.2f};
            }
            continue;
        }

        float dx = p->x - e->x;
        float dy = p->y - e->y;
        float dist = sqrtf(dx * dx + dy * dy);

        switch (e->state) {
        case AI_IDLE:
            if (dist < e->aggro_radius) {
                e->state = AI_CHASE;
            } else {
                e->attack_timer += delta;
                if (e->attack_timer >= 3.0f) {
                    e->attack_timer = 0.0f;
                    float ptdx = e->patrol_target_x - e->spawn_x;
                    float ptdy = e->patrol_target_y - e->spawn_y;
                    float patrol_angle = (float)i * 0.7854f;
                    if (fabsf(ptdx) > 0.1f || fabsf(ptdy) > 0.1f) {
                        patrol_angle = atan2f(ptdy, ptdx) + 2.3f;
                    }
                    e->patrol_target_x = e->spawn_x + cosf(patrol_angle) * 3.0f;
                    e->patrol_target_y = e->spawn_y + sinf(patrol_angle) * 3.0f;
                    if (e->patrol_target_x < 0.5f)
                        e->patrol_target_x = 0.5f;
                    if (e->patrol_target_x > (float)MAP_WIDTH - 0.5f)
                        e->patrol_target_x = (float)MAP_WIDTH - 0.5f;
                    if (e->patrol_target_y < 0.5f)
                        e->patrol_target_y = 0.5f;
                    if (e->patrol_target_y > (float)MAP_HEIGHT - 0.5f)
                        e->patrol_target_y = (float)MAP_HEIGHT - 0.5f;
                    e->state = AI_PATROL;
                }
            }
            break;

        case AI_PATROL: {
            if (dist < e->aggro_radius) {
                e->state = AI_CHASE;
                break;
            }

            float pdx = e->patrol_target_x - e->x;
            float pdy = e->patrol_target_y - e->y;
            float pdist = sqrtf(pdx * pdx + pdy * pdy);

            if (pdist < 0.2f) {
                e->state = AI_IDLE;
                e->attack_timer = 0.0f;
            } else {
                float move_speed = e->speed * 0.5f * delta;
                float nx = pdx / pdist;
                float ny = pdy / pdist;
                float new_x = e->x + nx * move_speed;
                float new_y = e->y + ny * move_speed;
                if (!MapIsSolid(state, new_x, new_y)) {
                    e->x = new_x;
                    e->y = new_y;
                }
            }
            break;
        }

        case AI_CHASE: {
            if (dist > e->aggro_radius * 1.5f) {
                e->state = AI_IDLE;
                e->attack_timer = 0.0f;
                break;
            }
            if (dist <= e->attack_radius) {
                e->state = AI_ATTACK;
                e->attack_timer = 0.0f;
                break;
            }

            float move_speed = e->speed * delta;
            float nx = dx / dist;
            float ny = dy / dist;
            float new_x = e->x + nx * move_speed;
            float new_y = e->y + ny * move_speed;

            if (!MapIsCamp(state, new_x, new_y) &&
                !MapIsSolid(state, new_x, new_y)) {
                e->x = new_x;
                e->y = new_y;
            }
            break;
        }

        case AI_ATTACK: {
            e->attack_timer -= delta;
            if (e->attack_timer <= 0.0f) {
                p->hp -= (float)e->damage;
                e->attack_timer = e->attack_cooldown;
            }
            if (dist > e->attack_radius) {
                e->state = AI_CHASE;
            }
            break;
        }
        }

        switch (e->state) {
        case AI_ATTACK:
            AnimationSet(&e->anim, ANIM_ATTACK, 4, 0.12f);
            break;
        case AI_CHASE:
        case AI_PATROL:
            AnimationSet(&e->anim, ANIM_RUN, 6, 0.1f);
            break;
        default:
            AnimationSet(&e->anim, ANIM_IDLE, 4, 0.2f);
            break;
        }
        AnimationUpdate(&e->anim, delta);
    }

    state->enemy_count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].alive) {
            state->enemy_count++;
        }
    }
}
