#pragma once
#include "game.h"

void PlayerInit(Player* player);
void PlayerHandleInput(GameState* state, float delta);
void PlayerUpdate(GameState* state, float delta);
