#define RAYLIB_TESTER_IMPLEMENTATION
#include "raylib_tester.h"

#define CANVAS_WIDTH 640
#define CANVAS_HEIGHT 480
#define RECT_WIDTH 60
#define RECT_HEIGHT 40
#define RECT_SPEED 100.0F
#define MOUSE_INDICATOR_RADIUS 5.0F

int main(int argc, char* argv[]) {
  const char* script_path = NULL;
  if (argc > 1) {
    script_path = argv[1];
  }

  InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "Game");
  SetTargetFPS(60);

  RltScriptRunner* runner = NULL;
  if (script_path != NULL) {
    runner = RltInitScriptRunner(script_path);
    if (runner == NULL) {
      CloseWindow();
      return 1;
    }
  }

  static float rect_x = (float)(CANVAS_WIDTH - RECT_WIDTH) / 2.0F;
  static float rect_y = (float)(CANVAS_HEIGHT - RECT_HEIGHT) / 2.0F;
  RltRegisterVar("rect_x", &rect_x, RLT_VAR_FLOAT);
  RltRegisterVar("rect_y", &rect_y, RLT_VAR_FLOAT);

  while (!WindowShouldClose() && !RltShouldClose()) {
    float delta = GetFrameTime();
    if (RltIsKeyDown(KEY_LEFT)) {
      rect_x -= RECT_SPEED * delta;
    }
    if (RltIsKeyDown(KEY_RIGHT)) {
      rect_x += RECT_SPEED * delta;
    }
    if (RltIsKeyPressed(KEY_SPACE)) {
      rect_x = (float)(CANVAS_WIDTH - RECT_WIDTH) / 2.0F;
    }

    if (rect_x < 0.0F) {
      rect_x = 0.0F;
    }
    if ((rect_x + RECT_WIDTH) > CANVAS_WIDTH) {
      rect_x = (float)(CANVAS_WIDTH - RECT_WIDTH);
    }

    RltClearSimOneshot();

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawRectangle((int)rect_x, (int)rect_y, RECT_WIDTH, RECT_HEIGHT, RED);

    Vector2 mouse_pos = GetMousePosition();
    Color mouse_color = BLUE;
    if (RltIsMouseButtonDown(MOUSE_BUTTON_LEFT) || RltIsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      mouse_color = RED;
    }
    DrawCircleV(mouse_pos, MOUSE_INDICATOR_RADIUS, mouse_color);

    DrawFPS(0, 0);
    EndDrawing();

    if (runner != NULL) {
      RltUpdateScriptRunner(runner);
    }
  }

  bool test_failed = (runner != NULL) && RltScriptRunnerHadError(runner);
  if (runner != NULL) {
    RltCloseScriptRunner(runner);
  }
  CloseWindow();
  return test_failed ? 1 : 0;
}
