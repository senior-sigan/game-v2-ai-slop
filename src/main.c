#include <raylib.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define CANVAS_WIDTH 640
#define CANVAS_HEIGHT 480
#define RECT_WIDTH 60
#define RECT_HEIGHT 40
#define RECT_SPEED 100.0F
#define MAX_KEYS 512
#define MAX_MOUSE_BUTTONS 8
#define MOUSE_INDICATOR_RADIUS 5.0F

static bool g_should_close = false;

// --- Game variable registry ---

typedef enum {
  VAR_FLOAT,
  VAR_INT,
  VAR_BOOL,
  VAR_DOUBLE,
} VarType;

typedef struct {
  const char *name;
  void *ptr;
  VarType type;
} GameVar;

#define MAX_GAME_VARS 64
static GameVar g_game_vars[MAX_GAME_VARS];
static int g_game_var_count = 0;

static void GameRegisterVar(const char *name, void *ptr, VarType type) {
  if (g_game_var_count < MAX_GAME_VARS) {
    g_game_vars[g_game_var_count].name = name;
    g_game_vars[g_game_var_count].ptr = ptr;
    g_game_vars[g_game_var_count].type = type;
    g_game_var_count++;
  }
}

// --- Simulated input state ---

static bool g_sim_keys[MAX_KEYS];
static bool g_sim_keys_prev[MAX_KEYS];
static bool g_sim_keys_oneshot[MAX_KEYS];

static bool g_sim_mouse[MAX_MOUSE_BUTTONS];
static bool g_sim_mouse_prev[MAX_MOUSE_BUTTONS];
static bool g_sim_mouse_oneshot[MAX_MOUSE_BUTTONS];

// --- Game input (real + simulated) ---

static bool GameIsKeyDown(int key) {
  if (IsKeyDown(key)) {
    return true;
  }
  if ((key >= 0) && (key < MAX_KEYS)) {
    return g_sim_keys[key];
  }
  return false;
}

static bool GameIsKeyPressed(int key) {
  if (IsKeyPressed(key)) {
    return true;
  }
  if ((key >= 0) && (key < MAX_KEYS)) {
    if (g_sim_keys[key]) {
      if (!g_sim_keys_prev[key]) {
        return true;
      }
    }
  }
  return false;
}

static bool GameIsMouseButtonDown(int btn) {
  if (IsMouseButtonDown(btn)) {
    return true;
  }
  if ((btn >= 0) && (btn < MAX_MOUSE_BUTTONS)) {
    return g_sim_mouse[btn];
  }
  return false;
}

static bool GameIsMouseButtonPressed(int btn) {
  if (IsMouseButtonPressed(btn)) {
    return true;
  }
  if ((btn >= 0) && (btn < MAX_MOUSE_BUTTONS)) {
    if (g_sim_mouse[btn]) {
      if (!g_sim_mouse_prev[btn]) {
        return true;
      }
    }
  }
  return false;
}

// Called after game logic reads input — release oneshot keys/buttons
static void ClearSimOneshot(void) {
  for (int idx = 0; idx < MAX_KEYS; idx++) {
    if (g_sim_keys_oneshot[idx]) {
      g_sim_keys[idx] = false;
      g_sim_keys_oneshot[idx] = false;
    }
  }
  for (int idx = 0; idx < MAX_MOUSE_BUTTONS; idx++) {
    if (g_sim_mouse_oneshot[idx]) {
      g_sim_mouse[idx] = false;
      g_sim_mouse_oneshot[idx] = false;
    }
  }
}

// Called after render, before Lua — save current as prev
static void SaveSimPrevState(void) {
  memcpy(g_sim_keys_prev, g_sim_keys, sizeof(g_sim_keys));
  memcpy(g_sim_mouse_prev, g_sim_mouse, sizeof(g_sim_mouse));
}

// --- Lua bindings ---

static int LuaTakeScreenshot(lua_State *state) {
  const char *filename = luaL_checkstring(state, 1);
  TakeScreenshot(filename);
  return 0;
}

static int LuaGetTime(lua_State *state) {
  lua_pushnumber(state, GetTime());
  return 1;
}

static int LuaCloseGame(lua_State *state) {
  (void)state;
  g_should_close = true;
  return 0;
}

static int LuaKeyDown(lua_State *state) {
  int key = (int)luaL_checkinteger(state, 1);
  if ((key >= 0) && (key < MAX_KEYS)) {
    g_sim_keys[key] = true;
  }
  return 0;
}

static int LuaKeyUp(lua_State *state) {
  int key = (int)luaL_checkinteger(state, 1);
  if ((key >= 0) && (key < MAX_KEYS)) {
    g_sim_keys[key] = false;
  }
  return 0;
}

static int LuaKeyPress(lua_State *state) {
  int key = (int)luaL_checkinteger(state, 1);
  if ((key >= 0) && (key < MAX_KEYS)) {
    g_sim_keys[key] = true;
    g_sim_keys_oneshot[key] = true;
  }
  return 0;
}

static int LuaSetMousePos(lua_State *state) {
  int pos_x = (int)luaL_checkinteger(state, 1);
  int pos_y = (int)luaL_checkinteger(state, 2);
  SetMousePosition(pos_x, pos_y);
  return 0;
}

static int LuaMouseDown(lua_State *state) {
  int btn = (int)luaL_checkinteger(state, 1);
  if ((btn >= 0) && (btn < MAX_MOUSE_BUTTONS)) {
    g_sim_mouse[btn] = true;
  }
  return 0;
}

static int LuaMouseUp(lua_State *state) {
  int btn = (int)luaL_checkinteger(state, 1);
  if ((btn >= 0) && (btn < MAX_MOUSE_BUTTONS)) {
    g_sim_mouse[btn] = false;
  }
  return 0;
}

static int LuaMousePress(lua_State *state) {
  int btn = (int)luaL_checkinteger(state, 1);
  if ((btn >= 0) && (btn < MAX_MOUSE_BUTTONS)) {
    g_sim_mouse[btn] = true;
    g_sim_mouse_oneshot[btn] = true;
  }
  return 0;
}

static int LuaGetVar(lua_State *state) {
  const char *name = luaL_checkstring(state, 1);
  for (int idx = 0; idx < g_game_var_count; idx++) {
    if (strcmp(g_game_vars[idx].name, name) == 0) {
      switch (g_game_vars[idx].type) {
        case VAR_FLOAT:
          lua_pushnumber(state, (double)(*(float *)g_game_vars[idx].ptr));
          return 1;
        case VAR_DOUBLE:
          lua_pushnumber(state, *(double *)g_game_vars[idx].ptr);
          return 1;
        case VAR_INT:
          lua_pushinteger(state, *(int *)g_game_vars[idx].ptr);
          return 1;
        case VAR_BOOL:
          lua_pushboolean(state, *(bool *)g_game_vars[idx].ptr);
          return 1;
      }
    }
  }
  return luaL_error(state, "unknown game variable: %s", name);
}

static const luaL_Reg GAME_LIB[] = {
    {"take_screenshot", LuaTakeScreenshot},
    {"get_time", LuaGetTime},
    {"close", LuaCloseGame},
    {"key_down", LuaKeyDown},
    {"key_up", LuaKeyUp},
    {"key_press", LuaKeyPress},
    {"set_mouse_pos", LuaSetMousePos},
    {"mouse_down", LuaMouseDown},
    {"mouse_up", LuaMouseUp},
    {"mouse_press", LuaMousePress},
    {"get_var", LuaGetVar},
    {NULL, NULL},
};

static void RegisterConstInt(lua_State *state, const char *name, int value) {
  lua_pushinteger(state, value);
  lua_setfield(state, -2, name);
}

static void RegisterGameLib(lua_State *state) {
  luaL_newlib(state, GAME_LIB);

  RegisterConstInt(state, "KEY_LEFT", KEY_LEFT);
  RegisterConstInt(state, "KEY_RIGHT", KEY_RIGHT);
  RegisterConstInt(state, "KEY_UP", KEY_UP);
  RegisterConstInt(state, "KEY_DOWN", KEY_DOWN);
  RegisterConstInt(state, "KEY_SPACE", KEY_SPACE);
  RegisterConstInt(state, "KEY_ENTER", KEY_ENTER);
  RegisterConstInt(state, "KEY_ESCAPE", KEY_ESCAPE);

  RegisterConstInt(state, "MOUSE_LEFT", MOUSE_BUTTON_LEFT);
  RegisterConstInt(state, "MOUSE_RIGHT", MOUSE_BUTTON_RIGHT);
  RegisterConstInt(state, "MOUSE_MIDDLE", MOUSE_BUTTON_MIDDLE);

  lua_setglobal(state, "game");
}

// --- Coroutine-based script runner ---

typedef struct {
  lua_State *main_state;
  lua_State *coroutine;
  double resume_at;
  bool finished;
  bool had_error;
} ScriptRunner;

static bool InitScriptRunner(ScriptRunner *runner, const char *path) {
  runner->main_state = luaL_newstate();
  runner->coroutine = NULL;
  runner->resume_at = 0.0;
  runner->finished = false;
  runner->had_error = false;

  luaL_openlibs(runner->main_state);
  RegisterGameLib(runner->main_state);

  if (luaL_dofile(runner->main_state, path) != LUA_OK) {
    (void)fprintf(
        stderr, "Lua load error: %s\n",
        lua_tostring(runner->main_state, -1));
    lua_close(runner->main_state);
    runner->main_state = NULL;
    return false;
  }

  runner->coroutine = lua_newthread(runner->main_state);

  lua_getglobal(runner->coroutine, "test");
  if (!lua_isfunction(runner->coroutine, -1)) {
    (void)fprintf(stderr, "Lua error: test() function not found in script\n");
    lua_close(runner->main_state);
    runner->main_state = NULL;
    return false;
  }

  return true;
}

static void UpdateScriptRunner(ScriptRunner *runner) {
  if (runner->finished) {
    return;
  }
  if (GetTime() < runner->resume_at) {
    return;
  }

  int nresults = 0;
#if LUA_VERSION_NUM >= 504
  int status =
      lua_resume(runner->coroutine, runner->main_state, 0, &nresults);
#else
  int status = lua_resume(runner->coroutine, runner->main_state, 0);
  nresults = lua_gettop(runner->coroutine);
#endif

  if (status == LUA_YIELD) {
    double seconds = 0.0;
    if (nresults >= 1 && lua_isnumber(runner->coroutine, -1)) {
      seconds = lua_tonumber(runner->coroutine, -1);
    }
    runner->resume_at = GetTime() + seconds;
    lua_pop(runner->coroutine, nresults);
  } else if (status == LUA_OK) {
    runner->finished = true;
    lua_pop(runner->coroutine, nresults);
  } else {
    (void)fprintf(
        stderr, "Lua runtime error: %s\n",
        lua_tostring(runner->coroutine, -1));
    runner->finished = true;
    runner->had_error = true;
    g_should_close = true;
  }
}

static void CloseScriptRunner(ScriptRunner *runner) {
  if (runner->main_state != NULL) {
    lua_close(runner->main_state);
    runner->main_state = NULL;
    runner->coroutine = NULL;
  }
}

// --- Main ---

int main(int argc, char *argv[]) {
  const char *script_path = NULL;
  if (argc > 1) {
    script_path = argv[1];
  }

  InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "Game");
  SetTargetFPS(60);

  ScriptRunner runner = {NULL, NULL, 0.0, false, false};
  bool has_script = false;
  if (script_path != NULL) {
    has_script = InitScriptRunner(&runner, script_path);
    if (!has_script) {
      CloseWindow();
      return 1;
    }
  }

  static float rect_x = (float)(CANVAS_WIDTH - RECT_WIDTH) / 2.0F;
  static float rect_y = (float)(CANVAS_HEIGHT - RECT_HEIGHT) / 2.0F;
  GameRegisterVar("rect_x", &rect_x, VAR_FLOAT);
  GameRegisterVar("rect_y", &rect_y, VAR_FLOAT);

  while (!WindowShouldClose() && !g_should_close) {
    // --- Game logic: read input ---
    float delta = GetFrameTime();
    if (GameIsKeyDown(KEY_LEFT)) {
      rect_x -= RECT_SPEED * delta;
    }
    if (GameIsKeyDown(KEY_RIGHT)) {
      rect_x += RECT_SPEED * delta;
    }
    if (GameIsKeyPressed(KEY_SPACE)) {
      rect_x = (float)(CANVAS_WIDTH - RECT_WIDTH) / 2.0F;
    }

    if (rect_x < 0.0F) {
      rect_x = 0.0F;
    }
    if ((rect_x + RECT_WIDTH) > CANVAS_WIDTH) {
      rect_x = (float)(CANVAS_WIDTH - RECT_WIDTH);
    }

    // --- Clear oneshot inputs (game logic already read them) ---
    ClearSimOneshot();

    // --- Render ---
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawRectangle((int)rect_x, (int)rect_y, RECT_WIDTH, RECT_HEIGHT, RED);

    // Mouse cursor indicator
    Vector2 mouse_pos = GetMousePosition();
    Color mouse_color = BLUE;
    if (GameIsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
        GameIsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      mouse_color = RED;
    }
    DrawCircleV(mouse_pos, MOUSE_INDICATOR_RADIUS, mouse_color);

    DrawFPS(0, 0);
    EndDrawing();

    // --- Save prev state before Lua may modify it ---
    SaveSimPrevState();

    // --- Run Lua script ---
    if (has_script) {
      UpdateScriptRunner(&runner);
    }
  }

  bool test_failed = has_script && runner.had_error;
  if (has_script) {
    CloseScriptRunner(&runner);
  }
  CloseWindow();
  return test_failed ? 1 : 0;
}
