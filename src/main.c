#include <raylib.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

#define CANVAS_WIDTH 640
#define CANVAS_HEIGHT 480

int main(void) {
  lua_State *lua = luaL_newstate();
  luaL_openlibs(lua);
  if (luaL_dostring(lua, "return 40 + 2") == LUA_OK) {
    printf("Lua result: %g\n", lua_tonumber(lua, -1));
    lua_pop(lua, 1);
  }
  lua_close(lua);

  InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "Game");
  SetTargetFPS(60);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawFPS(0, 0);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
