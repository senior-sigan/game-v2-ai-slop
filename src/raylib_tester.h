/*
 * raylib_tester.h — Lua-based test runner for raylib games (stb-style header)
 *
 * Usage:
 *   #define RAYLIB_TESTER_IMPLEMENTATION
 *   #include "raylib_tester.h"
 *
 * Before including the implementation, you may define:
 *   #define RLT_MAX_KEYS           512
 *   #define RLT_MAX_MOUSE_BUTTONS  8
 *   #define RLT_MAX_GAME_VARS      64
 */

#ifndef RAYLIB_TESTER_H
#define RAYLIB_TESTER_H

#include <raylib.h>
#include <stdbool.h>

#ifndef RLT_MAX_KEYS
#define RLT_MAX_KEYS 512
#endif

#ifndef RLT_MAX_MOUSE_BUTTONS
#define RLT_MAX_MOUSE_BUTTONS 8
#endif

#ifndef RLT_MAX_GAME_VARS
#define RLT_MAX_GAME_VARS 64
#endif

/* --- Variable registry --- */

typedef enum {
  RLT_VAR_FLOAT,
  RLT_VAR_INT,
  RLT_VAR_BOOL,
  RLT_VAR_DOUBLE,
} RltVarType;

void RltRegisterVar(const char* name, void* ptr, RltVarType type);

/* --- Input (real + simulated) --- */

bool RltIsKeyDown(int key);
bool RltIsKeyPressed(int key);
bool RltIsMouseButtonDown(int btn);
bool RltIsMouseButtonPressed(int btn);

void RltClearSimOneshot(void);

/* --- Should-close flag (set by Lua game.close()) --- */

bool RltShouldClose(void);

/* --- Script runner --- */

typedef struct RltScriptRunner RltScriptRunner;

RltScriptRunner* RltInitScriptRunner(const char* path);
void RltUpdateScriptRunner(RltScriptRunner* runner);
bool RltScriptRunnerFinished(const RltScriptRunner* runner);
bool RltScriptRunnerHadError(const RltScriptRunner* runner);
void RltCloseScriptRunner(RltScriptRunner* runner);

#endif /* RAYLIB_TESTER_H */

/* ======================================================================== */
/*                          IMPLEMENTATION                                  */
/* ======================================================================== */

#ifdef RAYLIB_TESTER_IMPLEMENTATION

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* --- Soft-assert counters --- */

static int rlt__assert_total = 0;
static int rlt__assert_failed = 0;

static void RltAssertFail(lua_State* state, const char* msg) {
  rlt__assert_failed++;
  luaL_where(state, 1);
  const char* where = lua_tostring(state, -1);
  (void)fprintf(stderr, "ASSERT FAILED [%s]: %s\n", where, msg);
  lua_pop(state, 1);
}

/* --- Should-close flag --- */

static bool rlt__should_close = false;

bool RltShouldClose(void) {
  return rlt__should_close;
}

/* --- Game variable registry --- */

typedef struct {
  const char* name;
  void* ptr;
  RltVarType type;
} RltGameVar;

static RltGameVar rlt__vars[RLT_MAX_GAME_VARS];
static int rlt__var_count = 0;

void RltRegisterVar(const char* name, void* ptr, RltVarType type) {
  if (rlt__var_count < RLT_MAX_GAME_VARS) {
    rlt__vars[rlt__var_count].name = name;
    rlt__vars[rlt__var_count].ptr = ptr;
    rlt__vars[rlt__var_count].type = type;
    rlt__var_count++;
  }
}

/* --- Simulated input state --- */

static bool rlt__sim_keys[RLT_MAX_KEYS];
static bool rlt__sim_keys_prev[RLT_MAX_KEYS];
static bool rlt__sim_keys_oneshot[RLT_MAX_KEYS];

static bool rlt__sim_mouse[RLT_MAX_MOUSE_BUTTONS];
static bool rlt__sim_mouse_prev[RLT_MAX_MOUSE_BUTTONS];
static bool rlt__sim_mouse_oneshot[RLT_MAX_MOUSE_BUTTONS];

bool RltIsKeyDown(int key) {
  if (IsKeyDown(key)) {
    return true;
  }
  if ((key >= 0) && (key < RLT_MAX_KEYS)) {
    return rlt__sim_keys[key];
  }
  return false;
}

bool RltIsKeyPressed(int key) {
  if (IsKeyPressed(key)) {
    return true;
  }
  if ((key >= 0) && (key < RLT_MAX_KEYS)) {
    if (rlt__sim_keys[key] && !rlt__sim_keys_prev[key]) {
      return true;
    }
  }
  return false;
}

bool RltIsMouseButtonDown(int btn) {
  if (IsMouseButtonDown(btn)) {
    return true;
  }
  if ((btn >= 0) && (btn < RLT_MAX_MOUSE_BUTTONS)) {
    return rlt__sim_mouse[btn];
  }
  return false;
}

bool RltIsMouseButtonPressed(int btn) {
  if (IsMouseButtonPressed(btn)) {
    return true;
  }
  if ((btn >= 0) && (btn < RLT_MAX_MOUSE_BUTTONS)) {
    if (rlt__sim_mouse[btn] && !rlt__sim_mouse_prev[btn]) {
      return true;
    }
  }
  return false;
}

void RltClearSimOneshot(void) {
  for (int idx = 0; idx < RLT_MAX_KEYS; idx++) {
    if (rlt__sim_keys_oneshot[idx]) {
      rlt__sim_keys[idx] = false;
      rlt__sim_keys_oneshot[idx] = false;
    }
  }
  for (int idx = 0; idx < RLT_MAX_MOUSE_BUTTONS; idx++) {
    if (rlt__sim_mouse_oneshot[idx]) {
      rlt__sim_mouse[idx] = false;
      rlt__sim_mouse_oneshot[idx] = false;
    }
  }
}

static void RltSaveSimPrevState(void) {
  memcpy(rlt__sim_keys_prev, rlt__sim_keys, sizeof(rlt__sim_keys));
  memcpy(rlt__sim_mouse_prev, rlt__sim_mouse, sizeof(rlt__sim_mouse));
}

/* --- Lua bindings --- */

static int RltLuaTakeScreenshot(lua_State* state) {
  const char* filename = luaL_checkstring(state, 1);
  TakeScreenshot(filename);
  return 0;
}

static int RltLuaGetTime(lua_State* state) {
  lua_pushnumber(state, GetTime());
  return 1;
}

static int RltLuaClose(lua_State* state) {
  (void)state;
  rlt__should_close = true;
  return 0;
}

static int RltLuaKeyDown(lua_State* state) {
  int key = (int)luaL_checkinteger(state, 1);
  if ((key >= 0) && (key < RLT_MAX_KEYS)) {
    rlt__sim_keys[key] = true;
  }
  return 0;
}

static int RltLuaKeyUp(lua_State* state) {
  int key = (int)luaL_checkinteger(state, 1);
  if ((key >= 0) && (key < RLT_MAX_KEYS)) {
    rlt__sim_keys[key] = false;
  }
  return 0;
}

static int RltLuaKeyPress(lua_State* state) {
  int key = (int)luaL_checkinteger(state, 1);
  if ((key >= 0) && (key < RLT_MAX_KEYS)) {
    rlt__sim_keys[key] = true;
    rlt__sim_keys_oneshot[key] = true;
  }
  return 0;
}

static int RltLuaSetMousePos(lua_State* state) {
  int pos_x = (int)luaL_checkinteger(state, 1);
  int pos_y = (int)luaL_checkinteger(state, 2);
  SetMousePosition(pos_x, pos_y);
  return 0;
}

static int RltLuaMouseDown(lua_State* state) {
  int btn = (int)luaL_checkinteger(state, 1);
  if ((btn >= 0) && (btn < RLT_MAX_MOUSE_BUTTONS)) {
    rlt__sim_mouse[btn] = true;
  }
  return 0;
}

static int RltLuaMouseUp(lua_State* state) {
  int btn = (int)luaL_checkinteger(state, 1);
  if ((btn >= 0) && (btn < RLT_MAX_MOUSE_BUTTONS)) {
    rlt__sim_mouse[btn] = false;
  }
  return 0;
}

static int RltLuaMousePress(lua_State* state) {
  int btn = (int)luaL_checkinteger(state, 1);
  if ((btn >= 0) && (btn < RLT_MAX_MOUSE_BUTTONS)) {
    rlt__sim_mouse[btn] = true;
    rlt__sim_mouse_oneshot[btn] = true;
  }
  return 0;
}

static int RltLuaGetVar(lua_State* state) {
  const char* name = luaL_checkstring(state, 1);
  for (int idx = 0; idx < rlt__var_count; idx++) {
    if (strcmp(rlt__vars[idx].name, name) == 0) {
      switch (rlt__vars[idx].type) {
        case RLT_VAR_FLOAT:
          lua_pushnumber(state, (double)(*(float*)rlt__vars[idx].ptr));
          return 1;
        case RLT_VAR_DOUBLE:
          lua_pushnumber(state, *(double*)rlt__vars[idx].ptr);
          return 1;
        case RLT_VAR_INT:
          lua_pushinteger(state, *(int*)rlt__vars[idx].ptr);
          return 1;
        case RLT_VAR_BOOL:
          lua_pushboolean(state, *(bool*)rlt__vars[idx].ptr);
          return 1;
      }
    }
  }
  return luaL_error(state, "unknown game variable: %s", name);
}

static int RltLuaAssertTrue(lua_State* state) {
  const char* msg = luaL_optstring(state, 2, "assert_true failed");
  rlt__assert_total++;
  if (!lua_toboolean(state, 1)) {
    RltAssertFail(state, msg);
  }
  return 0;
}

static int RltLuaAssertEq(lua_State* state) {
  const char* msg = luaL_optstring(state, 3, "assert_eq failed");
  rlt__assert_total++;
  int equal = lua_compare(state, 1, 2, LUA_OPEQ);
  if (!equal) {
    RltAssertFail(state, msg);
  }
  return 0;
}

static int RltLuaAssertNear(lua_State* state) {
  double val_a = luaL_checknumber(state, 1);
  double val_b = luaL_checknumber(state, 2);
  double eps = luaL_checknumber(state, 3);
  const char* msg = luaL_optstring(state, 4, "assert_near failed");
  rlt__assert_total++;
  if (fabs(val_a - val_b) > eps) {
    RltAssertFail(state, msg);
  }
  return 0;
}

static const luaL_Reg RLT_GAME_LIB[] = {
    {"take_screenshot", RltLuaTakeScreenshot},
    {"get_time", RltLuaGetTime},
    {"close", RltLuaClose},
    {"key_down", RltLuaKeyDown},
    {"key_up", RltLuaKeyUp},
    {"key_press", RltLuaKeyPress},
    {"set_mouse_pos", RltLuaSetMousePos},
    {"mouse_down", RltLuaMouseDown},
    {"mouse_up", RltLuaMouseUp},
    {"mouse_press", RltLuaMousePress},
    {"get_var", RltLuaGetVar},
    {"assert_true", RltLuaAssertTrue},
    {"assert_eq", RltLuaAssertEq},
    {"assert_near", RltLuaAssertNear},
    {NULL, NULL},
};

static void RltRegisterConstInt(lua_State* state, const char* name, int value) {
  lua_pushinteger(state, value);
  lua_setfield(state, -2, name);
}

static void RltRegisterGameLib(lua_State* state) {
  luaL_newlib(state, RLT_GAME_LIB);

  RltRegisterConstInt(state, "KEY_LEFT", KEY_LEFT);
  RltRegisterConstInt(state, "KEY_RIGHT", KEY_RIGHT);
  RltRegisterConstInt(state, "KEY_UP", KEY_UP);
  RltRegisterConstInt(state, "KEY_DOWN", KEY_DOWN);
  RltRegisterConstInt(state, "KEY_SPACE", KEY_SPACE);
  RltRegisterConstInt(state, "KEY_ENTER", KEY_ENTER);
  RltRegisterConstInt(state, "KEY_ESCAPE", KEY_ESCAPE);
  RltRegisterConstInt(state, "KEY_W", KEY_W);
  RltRegisterConstInt(state, "KEY_A", KEY_A);
  RltRegisterConstInt(state, "KEY_S", KEY_S);
  RltRegisterConstInt(state, "KEY_D", KEY_D);
  RltRegisterConstInt(state, "KEY_R", KEY_R);

  RltRegisterConstInt(state, "MOUSE_LEFT", MOUSE_BUTTON_LEFT);
  RltRegisterConstInt(state, "MOUSE_RIGHT", MOUSE_BUTTON_RIGHT);
  RltRegisterConstInt(state, "MOUSE_MIDDLE", MOUSE_BUTTON_MIDDLE);

  lua_setglobal(state, "game");
}

/* --- Script runner --- */

struct RltScriptRunner {
  lua_State* main_state;
  lua_State* coroutine;
  double resume_at;
  bool finished;
  bool had_error;
};

RltScriptRunner* RltInitScriptRunner(const char* path) {
  static RltScriptRunner runner;
  runner.main_state = luaL_newstate();
  runner.coroutine = NULL;
  runner.resume_at = 0.0;
  runner.finished = false;
  runner.had_error = false;

  luaL_openlibs(runner.main_state);
  RltRegisterGameLib(runner.main_state);

  if (luaL_dofile(runner.main_state, path) != LUA_OK) {
    (void)fprintf(stderr, "Lua load error: %s\n", lua_tostring(runner.main_state, -1));
    lua_close(runner.main_state);
    runner.main_state = NULL;
    return NULL;
  }

  runner.coroutine = lua_newthread(runner.main_state);

  lua_getglobal(runner.coroutine, "test");
  if (!lua_isfunction(runner.coroutine, -1)) {
    (void)fprintf(stderr, "Lua error: test() function not found in script\n");
    lua_close(runner.main_state);
    runner.main_state = NULL;
    return NULL;
  }

  return &runner;
}

void RltUpdateScriptRunner(RltScriptRunner* runner) {
  RltSaveSimPrevState();

  if (runner->finished) {
    return;
  }
  if (GetTime() < runner->resume_at) {
    return;
  }

  int nresults = 0;
#if LUA_VERSION_NUM >= 504
  int status = lua_resume(runner->coroutine, runner->main_state, 0, &nresults);
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
    int passed = rlt__assert_total - rlt__assert_failed;
    (void)fprintf(
        stderr,
        "Tests: %d passed, %d failed (%d total)\n",
        passed,
        rlt__assert_failed,
        rlt__assert_total
    );
    if (rlt__assert_failed > 0) {
      runner->had_error = true;
    }
  } else {
    (void)fprintf(stderr, "Lua runtime error: %s\n", lua_tostring(runner->coroutine, -1));
    runner->finished = true;
    runner->had_error = true;
    rlt__should_close = true;
  }
}

bool RltScriptRunnerFinished(const RltScriptRunner* runner) {
  return runner->finished;
}

bool RltScriptRunnerHadError(const RltScriptRunner* runner) {
  return runner->had_error;
}

void RltCloseScriptRunner(RltScriptRunner* runner) {
  if (runner->main_state != NULL) {
    lua_close(runner->main_state);
    runner->main_state = NULL;
    runner->coroutine = NULL;
  }
}

#endif /* RAYLIB_TESTER_IMPLEMENTATION */
