#define RAYLIB_TESTER_IMPLEMENTATION
#include "raylib_tester.h"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#include "combat.h"
#include "enemy.h"
#include "game.h"
#include "map.h"
#include "player.h"
#include "render.h"
#include "sprites.h"

static SpriteAssets sprites = {0};
static GameState state = {0};

#ifndef PLATFORM_WEB
static RltScriptRunner* runner = NULL;
#endif

static void Update(void) {
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

#ifndef PLATFORM_WEB
  if (runner != NULL) {
    RltUpdateScriptRunner(runner);
  }
#endif
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  InitWindow(800, 600, "Dark Camp");
  SetTargetFPS(60);

  SpritesInit(&sprites);

  MapInit(&state);
  PlayerInit(&state.player);
  EnemiesInit(&state);
  state.running = true;
  state.camera_x = state.player.x;
  state.camera_y = state.player.y;

#ifndef PLATFORM_WEB
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
#endif

#ifdef PLATFORM_WEB
  emscripten_set_main_loop(Update, 0, 1);
#else
  while (!WindowShouldClose() && !RltShouldClose() && state.running) {
    Update();
  }

  bool test_failed = (runner != NULL) && RltScriptRunnerHadError(runner);
  if (runner != NULL) {
    RltCloseScriptRunner(runner);
  }
#endif

  SpritesUnload(&sprites);
  CloseWindow();

#ifndef PLATFORM_WEB
  return test_failed ? 1 : 0;
#else
  return 0;
#endif
}
