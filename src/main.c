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

// --- Lua script runner ---

static lua_State *LoadTestScript(const char *path) {
  lua_State *state = luaL_newstate();
  luaL_openlibs(state);
  RegisterGameLib(state);

  if (luaL_dofile(state, path) != LUA_OK) {
    (void)fprintf(stderr, "Lua error: %s\n", lua_tostring(state, -1));
    lua_close(state);
    return NULL;
  }
  return state;
}

static void CallLuaUpdate(lua_State *state) {
  lua_getglobal(state, "update");
  if (!lua_isfunction(state, -1)) {
    lua_pop(state, 1);
    return;
  }
  if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
    (void)fprintf(stderr, "Lua update error: %s\n", lua_tostring(state, -1));
    lua_pop(state, 1);
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

  lua_State *lua = NULL;
  if (script_path != NULL) {
    lua = LoadTestScript(script_path);
    if (lua == NULL) {
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

    if (lua != NULL) {
      CallLuaUpdate(lua);
    }
  }

  if (lua != NULL) {
    lua_close(lua);
  }
  CloseWindow();
  return 0;
}
