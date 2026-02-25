#define RAYLIB_TESTER_IMPLEMENTATION
#include "raylib_tester.h"
#include "game.h"
#include "player.h"
#include "enemy.h"
#include "map.h"
#include "combat.h"
#include "render.h"
#include "sprites.h"

int main(int argc, char* argv[]) {
    InitWindow(800, 600, "Dark Camp");
    SetTargetFPS(60);

    static SpriteAssets sprites = {0};
    SpritesInit(&sprites);

    static GameState state = {0};
    MapInit(&state);
    PlayerInit(&state.player);
    EnemiesInit(&state);
    state.running = true;
    state.camera_x = state.player.x;
    state.camera_y = state.player.y;

    RltScriptRunner* runner = NULL;
    if (argc > 1) {
        runner = RltInitScriptRunner(argv[1]);
        if (runner == NULL) {
            CloseWindow();
            return 1;
        }
    }

    RltRegisterVar("player_x", &state.player.x, RLT_VAR_FLOAT);
    RltRegisterVar("player_y", &state.player.y, RLT_VAR_FLOAT);
    RltRegisterVar("player_hp", &state.player.hp, RLT_VAR_FLOAT);
    RltRegisterVar("enemy_count", &state.enemy_count, RLT_VAR_INT);
    RltRegisterVar("player_kills", &state.player.kills, RLT_VAR_INT);

    while (!WindowShouldClose() && !RltShouldClose() && state.running) {
        float delta = GetFrameTime();

        PlayerHandleInput(&state, delta);
        PlayerUpdate(&state, delta);
        EnemiesUpdate(&state, delta);
        CombatUpdate(&state);

        state.camera_x = state.player.x;
        state.camera_y = state.player.y;

        RltClearSimOneshot();

        BeginDrawing();
        ClearBackground(DARKGRAY);
        RenderMap(&state, &sprites);
        RenderEnemies(&state, &sprites);
        RenderPlayer(&state, &sprites);
        RenderUI(&state);
        EndDrawing();

        if (runner != NULL) {
            RltUpdateScriptRunner(runner);
        }
    }

    bool test_failed = (runner != NULL) && RltScriptRunnerHadError(runner);
    if (runner != NULL) {
        RltCloseScriptRunner(runner);
    }
    SpritesUnload(&sprites);
    CloseWindow();
    return test_failed ? 1 : 0;
}
