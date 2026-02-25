#pragma once
#include "game.h"

void MapInit(GameState* state);
bool MapIsSolid(const GameState* state, float world_x, float world_y);
bool MapIsCamp(const GameState* state, float world_x, float world_y);
TileType MapGetTile(const GameState* state, int tile_x, int tile_y);
