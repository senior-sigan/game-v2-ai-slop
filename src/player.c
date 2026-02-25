#include "player.h"
#include "map.h"
#include "render.h"
#include "raylib_tester.h"
#include <math.h>

void PlayerInit(Player* player) {
    *player = (Player){
        .x = 15.0f,
        .y = 15.0f,
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

    float move_x = 0.0f;
    float move_y = 0.0f;

    if (RltIsKeyDown(KEY_W)) {
        move_x -= 1.0f;
        move_y -= 1.0f;
    }
    if (RltIsKeyDown(KEY_S)) {
        move_x += 1.0f;
        move_y += 1.0f;
    }
    if (RltIsKeyDown(KEY_A)) {
        move_x -= 1.0f;
        move_y += 1.0f;
    }
    if (RltIsKeyDown(KEY_D)) {
        move_x += 1.0f;
        move_y -= 1.0f;
    }

    float len = sqrtf(move_x * move_x + move_y * move_y);
    if (len > 0.0f) {
        move_x /= len;
        move_y /= len;

        float new_x = p->x + move_x * p->speed * delta;
        float new_y = p->y + move_y * p->speed * delta;

        float half = 0.4f;
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

    if (RltIsMouseButtonPressed(MOUSE_BUTTON_LEFT) && p->attack_timer <= 0.0f) {
        Vector2 mouse = GetMousePosition();
        float world_mx = 0.0f;
        float world_my = 0.0f;
        ScreenToWorld(mouse.x, mouse.y, state->camera_x, state->camera_y,
                      &world_mx, &world_my);

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

    if (p->attack_timer > 0.0f) {
        p->attack_timer -= delta;
        if (p->attack_timer <= 0.0f) {
            p->attack_timer = 0.0f;
            p->attacking = false;
        }
    }

    if (MapIsCamp(state, p->x, p->y)) {
        p->hp += CAMP_REGEN_RATE * delta;
        if (p->hp > p->max_hp) {
            p->hp = p->max_hp;
        }
    }
}
