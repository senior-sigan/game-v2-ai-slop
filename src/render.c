#include "render.h"
#include <math.h>

Vector2 WorldToScreen(float wx, float wy, float cam_x, float cam_y) {
    float rx = wx - cam_x;
    float ry = wy - cam_y;
    return (Vector2){
        .x = (rx - ry) * (float)ISO_SCALE_X + (float)SCREEN_CENTER_X,
        .y = (rx + ry) * (float)ISO_SCALE_Y + (float)SCREEN_CENTER_Y,
    };
}

void ScreenToWorld(float sx, float sy, float cam_x, float cam_y,
                   float* wx, float* wy) {
    float rx = sx - (float)SCREEN_CENTER_X;
    float ry = sy - (float)SCREEN_CENTER_Y;
    *wx = (rx / (float)ISO_SCALE_X + ry / (float)ISO_SCALE_Y) / 2.0f + cam_x;
    *wy = (ry / (float)ISO_SCALE_Y - rx / (float)ISO_SCALE_X) / 2.0f + cam_y;
}

static void DrawIsoDiamond(Vector2 center, Color color) {
    Vector2 top = {center.x, center.y - (float)ISO_SCALE_Y};
    Vector2 right = {center.x + (float)ISO_SCALE_X, center.y};
    Vector2 bottom = {center.x, center.y + (float)ISO_SCALE_Y};
    Vector2 left = {center.x - (float)ISO_SCALE_X, center.y};

    DrawTriangle(top, left, right, color);
    DrawTriangle(left, bottom, right, color);
}

static Texture2D GetEnemyTexture(const SpriteAssets* sprites,
                                 EnemyType type, AiState ai) {
    switch (type) {
    case ENEMY_SKELETON:
        switch (ai) {
        case AI_ATTACK: return sprites->skeleton_attack;
        case AI_CHASE:
        case AI_PATROL: return sprites->skeleton_run;
        default: return sprites->skeleton_idle;
        }
    case ENEMY_ZOMBIE:
        switch (ai) {
        case AI_ATTACK: return sprites->zombie_attack;
        case AI_CHASE:
        case AI_PATROL: return sprites->zombie_run;
        default: return sprites->zombie_idle;
        }
    case ENEMY_LICH:
        switch (ai) {
        case AI_ATTACK: return sprites->lich_attack;
        case AI_CHASE:
        case AI_PATROL: return sprites->lich_run;
        default: return sprites->lich_idle;
        }
    }
    return sprites->skeleton_idle;
}

void RenderMap(const GameState* state, const SpriteAssets* sprites) {
    for (int ty = 0; ty < MAP_HEIGHT; ty++) {
        for (int tx = 0; tx < MAP_WIDTH; tx++) {
            TileType tile = state->map[ty][tx];
            Vector2 center = WorldToScreen((float)tx, (float)ty,
                                           state->camera_x, state->camera_y);

            Texture2D tex = {0};
            Color fallback = GREEN;
            switch (tile) {
            case TILE_GRASS:
                tex = sprites->tile_grass;
                fallback = GREEN;
                break;
            case TILE_DIRT:
                tex = sprites->tile_dirt;
                fallback = BROWN;
                break;
            case TILE_GRAVE:
                tex = sprites->tile_grave;
                fallback = DARKGRAY;
                break;
            case TILE_CAMP:
                tex = sprites->tile_camp;
                fallback = BEIGE;
                break;
            }

            if (IsTextureValid(tex)) {
                Vector2 tile_pos = {center.x - 16.0f, center.y - 8.0f};
                DrawTextureV(tex, tile_pos, WHITE);
            } else {
                DrawIsoDiamond(center, fallback);
            }

            if (tile == TILE_GRAVE) {
                if (IsTextureValid(sprites->tombstone)) {
                    Vector2 dpos = {center.x - 8.0f, center.y - 14.0f};
                    DrawTextureV(sprites->tombstone, dpos, WHITE);
                } else {
                    DrawRectangle((int)center.x - 3, (int)center.y - 6,
                                  6, 10, GRAY);
                }
            }

            if (tile == TILE_GRASS && (tx * 7 + ty * 13) % 11 == 0) {
                if (IsTextureValid(sprites->tree)) {
                    Vector2 tpos = {center.x - 16.0f, center.y - 28.0f};
                    DrawTextureV(sprites->tree, tpos, WHITE);
                }
            }

            if (tile == TILE_DIRT && (tx * 11 + ty * 7) % 13 == 0) {
                if (IsTextureValid(sprites->bones)) {
                    Vector2 bpos = {center.x - 4.0f, center.y - 4.0f};
                    DrawTextureV(sprites->bones, bpos, WHITE);
                }
            }
        }
    }

    static Animation campfire_anim = {
        .frame_count = 8, .frame_duration = 0.1f};
    AnimationUpdate(&campfire_anim, GetFrameTime());

    Vector2 fire_pos = WorldToScreen(15.0f, 15.0f,
                                     state->camera_x, state->camera_y);
    if (IsTextureValid(sprites->campfire)) {
        Rectangle src = AnimationFrame(&campfire_anim, 16, 16);
        Rectangle dest = {fire_pos.x - 12.0f, fire_pos.y - 18.0f,
                          24.0f, 24.0f};
        DrawTexturePro(sprites->campfire, src, dest,
                       (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawCircleV(fire_pos, 5.0f, ORANGE);
        DrawCircleV(fire_pos, 3.0f, YELLOW);
    }
}

void RenderEnemies(const GameState* state, const SpriteAssets* sprites) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy* e = &state->enemies[i];
        if (e->alive) continue;

        Vector2 pos = WorldToScreen(e->x, e->y,
                                    state->camera_x, state->camera_y);
        if (IsTextureValid(sprites->bones)) {
            Vector2 bpos = {pos.x - 4.0f, pos.y - 4.0f};
            DrawTextureV(sprites->bones, bpos, WHITE);
        } else {
            DrawCircleV(pos, 4.0f, GRAY);
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy* e = &state->enemies[i];
        if (!e->alive) continue;

        Vector2 pos = WorldToScreen(e->x, e->y,
                                    state->camera_x, state->camera_y);

        Texture2D tex = GetEnemyTexture(sprites, e->type, e->state);

        if (IsTextureValid(tex)) {
            Rectangle src = AnimationFrame(&e->anim, 16, 16);
            Rectangle dest = {pos.x - 16.0f, pos.y - 28.0f, 32.0f, 32.0f};
            DrawTexturePro(tex, src, dest,
                           (Vector2){0, 0}, 0.0f, WHITE);
        } else {
            Color color = WHITE;
            float radius = 10.0f;
            switch (e->type) {
            case ENEMY_SKELETON:
                color = WHITE;
                radius = 10.0f;
                break;
            case ENEMY_ZOMBIE:
                color = GREEN;
                radius = 14.0f;
                break;
            case ENEMY_LICH:
                color = PURPLE;
                radius = 11.0f;
                break;
            }
            DrawCircleV(pos, radius, color);
        }

        float bar_width = 28.0f;
        float bar_height = 3.0f;
        float bar_x = pos.x - bar_width / 2.0f;
        float bar_y = pos.y - 34.0f;
        float hp_ratio = e->hp / e->max_hp;
        DrawRectangle((int)bar_x, (int)bar_y,
                      (int)bar_width, (int)bar_height, DARKGRAY);
        DrawRectangle((int)bar_x, (int)bar_y,
                      (int)(bar_width * hp_ratio), (int)bar_height, RED);
    }
}

void RenderPlayer(const GameState* state, const SpriteAssets* sprites) {
    const Player* p = &state->player;
    Vector2 pos = WorldToScreen(p->x, p->y, state->camera_x, state->camera_y);

    Texture2D tex = sprites->player_idle;
    if (p->attacking) {
        tex = sprites->player_attack;
    } else if (p->vx != 0.0f || p->vy != 0.0f) {
        tex = sprites->player_run;
    }

    if (IsTextureValid(tex)) {
        Rectangle src = AnimationFrame(&p->anim, 16, 16);
        Rectangle dest = {pos.x - 16.0f, pos.y - 28.0f, 32.0f, 32.0f};
        DrawTexturePro(tex, src, dest,
                       (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawCircleV(pos, 12.0f, BLUE);
    }

    if (p->attacking) {
        float world_dx = cosf(p->attack_angle);
        float world_dy = sinf(p->attack_angle);
        float screen_ax = (world_dx - world_dy) * (float)ISO_SCALE_X;
        float screen_ay = (world_dx + world_dy) * (float)ISO_SCALE_Y;
        float screen_angle = atan2f(screen_ay, screen_ax) * RAD2DEG;
        float half_arc_deg = (p->attack_arc / 2.0f) * RAD2DEG;
        DrawCircleSector(pos, 24.0f,
                         screen_angle - half_arc_deg,
                         screen_angle + half_arc_deg,
                         16, Fade(WHITE, 0.3f));
    }
}

void RenderUI(const GameState* state) {
    const Player* p = &state->player;

    DrawRectangle(10, 10, 200, 20, DARKGRAY);
    float hp_ratio = p->hp / p->max_hp;
    DrawRectangle(10, 10, (int)(200.0f * hp_ratio), 20, RED);
    DrawText(TextFormat("HP: %.0f/%.0f", (double)p->hp, (double)p->max_hp),
             15, 13, 16, WHITE);

    const char* kills_text = TextFormat("Kills: %d", p->kills);
    int text_width = MeasureText(kills_text, 20);
    DrawText(kills_text, 790 - text_width, 10, 20, WHITE);

    DrawFPS(10, 35);
}
