#pragma once
#include "game.h"
#include <raylib.h>

Vector2 WorldToScreen(float world_x, float world_y, float cam_x, float cam_y);
void ScreenToWorld(float sx, float sy, float cam_x, float cam_y,
                   float* wx, float* wy);

void RenderMap(const GameState* state);
void RenderEnemies(const GameState* state);
void RenderPlayer(const GameState* state);
void RenderUI(const GameState* state);
