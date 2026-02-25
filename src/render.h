#pragma once
#include "game.h"
#include "sprites.h"
#include <raylib.h>

Vector2 WorldToScreen(float world_x, float world_y, float cam_x, float cam_y);
void ScreenToWorld(float sx, float sy, float cam_x, float cam_y,
                   float* wx, float* wy);

void RenderMap(const GameState* state, const SpriteAssets* sprites);
void RenderEnemies(const GameState* state, const SpriteAssets* sprites);
void RenderPlayer(const GameState* state, const SpriteAssets* sprites);
void RenderUI(const GameState* state);
