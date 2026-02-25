#include <raylib.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>

#define CANVAS_WIDTH 640
#define CANVAS_HEIGHT 480
#define RECT_WIDTH 60
#define RECT_HEIGHT 40
#define RECT_SPEED 200.0F

static bool g_should_close = false;

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

static const luaL_Reg GAME_LIB[] = {
    {"take_screenshot", LuaTakeScreenshot},
    {"get_time", LuaGetTime},
    {"close", LuaCloseGame},
    {NULL, NULL},
};

static void RegisterGameLib(lua_State *state) {
  luaL_newlib(state, GAME_LIB);
  lua_setglobal(state, "game");
}

// --- Coroutine-based script runner ---

typedef struct {
  lua_State *main_state;
  lua_State *coroutine;
  double resume_at;
  bool finished;
} ScriptRunner;

static bool InitScriptRunner(ScriptRunner *runner, const char *path) {
  runner->main_state = luaL_newstate();
  runner->coroutine = NULL;
  runner->resume_at = 0.0;
  runner->finished = false;

  luaL_openlibs(runner->main_state);
  RegisterGameLib(runner->main_state);

  // Load script — it should define a test() function
  if (luaL_dofile(runner->main_state, path) != LUA_OK) {
    (void)fprintf(
        stderr, "Lua load error: %s\n",
        lua_tostring(runner->main_state, -1));
    lua_close(runner->main_state);
    runner->main_state = NULL;
    return false;
  }

  // Create a coroutine thread from test()
  runner->coroutine = lua_newthread(runner->main_state);
  // Thread is on main_state's stack — won't be GC'd while main_state lives

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

  ScriptRunner runner = {NULL, NULL, 0.0, false};
  bool has_script = false;
  if (script_path != NULL) {
    has_script = InitScriptRunner(&runner, script_path);
    if (!has_script) {
      CloseWindow();
      return 1;
    }
  }

  float rect_x = 0.0F;
  float rect_y = (float)(CANVAS_HEIGHT - RECT_HEIGHT) / 2.0F;
  int direction = 1;

  while (!WindowShouldClose() && !g_should_close) {
    float delta = GetFrameTime();
    rect_x += (RECT_SPEED * delta) * (float)direction;
    if ((rect_x + RECT_WIDTH) >= CANVAS_WIDTH) {
      rect_x = (float)(CANVAS_WIDTH - RECT_WIDTH);
      direction = -1;
    } else if (rect_x <= 0.0F) {
      rect_x = 0.0F;
      direction = 1;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawRectangle((int)rect_x, (int)rect_y, RECT_WIDTH, RECT_HEIGHT, RED);
    DrawFPS(0, 0);
    EndDrawing();

    if (has_script) {
      UpdateScriptRunner(&runner);
    }
  }

  if (has_script) {
    CloseScriptRunner(&runner);
  }
  CloseWindow();
  return 0;
}
