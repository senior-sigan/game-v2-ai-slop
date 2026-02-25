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

void RenderMap(const GameState* state) {
    for (int ty = 0; ty < MAP_HEIGHT; ty++) {
        for (int tx = 0; tx < MAP_WIDTH; tx++) {
            TileType tile = state->map[ty][tx];
            Vector2 center = WorldToScreen((float)tx, (float)ty,
                                           state->camera_x, state->camera_y);

            Color color = GREEN;
            switch (tile) {
            case TILE_GRASS:
                color = GREEN;
                break;
            case TILE_DIRT:
                color = BROWN;
                break;
            case TILE_GRAVE:
                color = DARKGRAY;
                break;
            case TILE_CAMP:
                color = BEIGE;
                break;
            }

            DrawIsoDiamond(center, color);

            if (tile == TILE_GRAVE) {
                DrawRectangle((int)center.x - 3, (int)center.y - 6, 6, 10, GRAY);
            }
        }
    }

    Vector2 fire_pos = WorldToScreen(15.0f, 15.0f,
                                     state->camera_x, state->camera_y);
    DrawCircleV(fire_pos, 5.0f, ORANGE);
    DrawCircleV(fire_pos, 3.0f, YELLOW);
}

void RenderEnemies(const GameState* state) {
    (void)state;
}

void RenderPlayer(const GameState* state) {
    const Player* p = &state->player;
    Vector2 pos = WorldToScreen(p->x, p->y, state->camera_x, state->camera_y);

    DrawCircleV(pos, 12.0f, BLUE);

    Vector2 mouse = GetMousePosition();
    float sdx = mouse.x - pos.x;
    float sdy = mouse.y - pos.y;
    float slen = sqrtf(sdx * sdx + sdy * sdy);
    if (slen > 0.0f) {
        sdx /= slen;
        sdy /= slen;
        Vector2 sword_end = {pos.x + sdx * 20.0f, pos.y + sdy * 20.0f};
        DrawLineEx(pos, sword_end, 2.0f, WHITE);
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
